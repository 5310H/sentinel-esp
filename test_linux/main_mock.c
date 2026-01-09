#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "mongoose.h"
#include "engine.h"
#include "storage_mgr.h"

// --- PROTOTYPES ---
void monitor_init(void);

// --- GLOBAL VARIABLES ---
static struct mg_mgr mgr;
int mock_gpio_pins[100] = { [0 ... 99] = 1 }; 
// Track relay states for the Part C status loop
bool mock_relay_states[MAX_RELAYS] = { false }; 

// --- HELPER FUNCTIONS ---
int digitalRead(int pin) {
    if (pin < 0 || pin >= 100) return 1;
    return mock_gpio_pins[pin];
}

void get_json_str(struct mg_str json, const char *path, char *dst, int dst_len) {
    char *val = mg_json_get_str(json, path);
    if (val != NULL) {
        snprintf(dst, dst_len, "%s", val);
        free(val);
    }
}

// --- MOCK HAL WRAPPERS (NOW CONNECTED) ---
void __wrap_hal_set_relay(int relay_id, bool active) { 
    if (relay_id >= 0 && relay_id < MAX_RELAYS) mock_relay_states[relay_id] = active;
    printf("[HAL] Relay %d -> %s\n", relay_id, active ? "ON" : "OFF");
}
bool __wrap_hal_get_relay_state(int relay_id) { 
    return (relay_id >= 0 && relay_id < MAX_RELAYS) ? mock_relay_states[relay_id] : false; 
}
void __wrap_hal_set_siren(bool active) { printf("[HAL] SIREN -> %s\n", active ? "ON" : "OFF"); }
void __wrap_hal_set_strobe(bool active) { printf("[HAL] STROBE -> %s\n", active ? "ON" : "OFF"); }
int __wrap_hal_get_zone_state(int zone_id) { return digitalRead(zone_id); }

// --- WEB HANDLER ---
static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        
        if (mg_strcmp(hm->uri, mg_str("/api/users")) == 0) {
            char *json = users_to_json(); 
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json);
            free(json);
        }
        else if (mg_strcmp(hm->uri, mg_str("/api/users/add")) == 0) {
            char name[STR_SMALL] = {0}, pin[STR_SMALL] = {0}, email[STR_MEDIUM] = {0}, n_buf[10] = {0};
            get_json_str(hm->body, "$.name", name, sizeof(name));
            get_json_str(hm->body, "$.pin", pin, sizeof(pin));
            get_json_str(hm->body, "$.email", email, sizeof(email));
            get_json_str(hm->body, "$.notify", n_buf, sizeof(n_buf));
            user_add(users, &u_count, name, pin, "", email, atoi(n_buf), false); 
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"ok\"}");
        }
        else if (mg_strcmp(hm->uri, mg_str("/api/auth")) == 0) {
            char pin_in[STR_SMALL] = {0};
            get_json_str(hm->body, "$.pin", pin_in, sizeof(pin_in));
            keypad_result_t res = engine_check_keypad(pin_in);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"authenticated\":%s,\"is_admin\":%s,\"name\":\"%s\",\"state\":%d}",
                res.authenticated ? "true" : "false", res.is_admin ? "true" : "false",
                res.name, engine_get_arm_state());
        }
        else if (mg_strcmp(hm->uri, mg_str("/api/status")) == 0) {
            bool is_ready = true;
            static char s_buf[45056]; 
            memset(s_buf, 0, sizeof(s_buf));

            for (int i = 0; i < z_count; i++) {
                if (zones[i].is_perimeter && digitalRead(zones[i].gpio) == 0) { is_ready = false; break; }
            }

            // --- Part A: GLOBAL CONFIG ---
            int off = snprintf(s_buf, sizeof(s_buf), 
                "{\"config\":{"
                "\"acct_id\":\"%s\",\"pin\":\"%s\",\"name\":\"%s\",\"addr1\":\"%s\",\"addr2\":\"%s\","
                "\"city\":\"%s\",\"state\":\"%s\",\"zip\":\"%s\",\"email\":\"%s\",\"phone\":\"%s\","
                "\"instr\":\"%s\",\"lat\":%f,\"lon\":%f,\"acc\":%d,"
                "\"mon_id\":\"%s\",\"mon_key\":\"%s\",\"mon_url\":\"%s\",\"notify\":%d,"
                "\"fire\":%s,\"police\":%s,\"med\":%s,\"oth\":%s,"
                "\"smtp_srv\":\"%s\",\"smtp_port\":%d,\"smtp_user\":\"%s\",\"smtp_pass\":\"%s\","
                "\"mqtt_srv\":\"%s\",\"mqtt_port\":%d,\"mqtt_user\":\"%s\",\"mqtt_pass\":\"%s\","
                "\"tg_id\":\"%s\",\"tg_tok\":\"%s\",\"tg_en\":%s,\"nvr_url\":\"%s\",\"ha_url\":\"%s\","
                "\"ent_d\":%d,\"ext_d\":%d,\"can_d\":%d,\"sys_state\":%d,\"ready\":%s},",
                config.account_id, config.pin, config.name, config.address1, config.address2,
                config.city, config.state, config.zip_code, config.email, config.phone,
                config.instructions, config.latitude, config.longitude, config.accuracy,
                config.monitor_service_id, config.monitor_service_key, config.monitoring_url, config.notify,
                config.is_monitor_fire ? "true" : "false", config.is_monitor_police ? "true" : "false",
                config.is_monitor_medical ? "true" : "false", config.is_monitor_other ? "true" : "false",
                config.smtp_server, config.smtp_port, config.smtp_user, config.smtp_pass,
                config.mqtt_server, config.mqtt_port, config.mqtt_user, config.mqtt_pass,
                config.telegram_id, config.telegram_token, config.is_telegram_enabled ? "true" : "false",
                config.nvrserver_url, config.haintegration_url,
                config.entry_delay, config.exit_delay, config.cancel_delay,
                engine_get_arm_state(), is_ready ? "true" : "false");

            // --- Part B: ZONES ARRAY ---
            off += snprintf(s_buf + off, sizeof(s_buf) - off, "\"zones\":[");
            for (int i = 0; i < z_count; i++) {
                off += snprintf(s_buf + off, sizeof(s_buf) - off, 
                    "{\"id\":%d,\"name\":\"%s\",\"desc\":\"%s\",\"type\":\"%s\",\"loc\":\"%s\","
                    "\"mod\":\"%s\",\"mfr\":\"%s\",\"chime\":%s,\"arm_only\":%s,\"gpio\":%d,"
                    "\"i2c\":%s,\"i2c_addr\":%d,\"peri\":%s,\"interior\":%s,\"panic\":%s,\"violated\":%s}%s", 
                    zones[i].id, zones[i].name, zones[i].description, zones[i].type, zones[i].location,
                    zones[i].model, zones[i].manufacturer, zones[i].is_chime ? "true" : "false",
                    zones[i].is_alarm_on_armed_only ? "true" : "false", zones[i].gpio,
                    zones[i].is_i2c ? "true" : "false", zones[i].i2c_address,
                    zones[i].is_perimeter ? "true" : "false", zones[i].is_interior ? "true" : "false",
                    zones[i].is_panic ? "true" : "false", (digitalRead(zones[i].gpio) == 0) ? "true" : "false",
                    (i < z_count - 1) ? "," : "");
            }

            // --- Part C: RELAYS ARRAY ---
            off += snprintf(s_buf + off, sizeof(s_buf) - off, "],\"relays\":[");
            for (int i = 0; i < r_count; i++) {
                off += snprintf(s_buf + off, sizeof(s_buf) - off, 
                    "{\"id\":%d,\"name\":\"%s\",\"desc\":\"%s\",\"dur\":%d,\"loc\":\"%s\","
                    "\"type\":\"%s\",\"repeat\":%s,\"gpio\":%d,\"active\":%s}%s", 
                    relays[i].id, relays[i].name, relays[i].description, relays[i].duration,
                    relays[i].location, relays[i].type, relays[i].is_repeat ? "true" : "false",
                    relays[i].gpio, mock_relay_states[relays[i].id] ? "true" : "false", 
                    (i < r_count - 1) ? "," : "");
            }
            strcat(s_buf, "]}");
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", s_buf);
        }
// --- NEW: ARM/DISARM HANDLERS ---
        else if (mg_strcmp(hm->uri, mg_str("/api/arm")) == 0) {
            // mode 1 is usually ARM_AWAY, mode 2 is ARM_STAY
            engine_ui_arm(1); 
            printf("[API] Arming Command Received\n");
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"ok\"}");
        }
        else if (mg_strcmp(hm->uri, mg_str("/api/disarm")) == 0) {
            engine_ui_disarm();
            // Clear mock relays on disarm
            for(int i=0; i<MAX_RELAYS; i++) mock_relay_states[i] = false;
            printf("[API] Disarm Command Received\n");
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"ok\"}");
        }

        // --- NEW: MOCK SENSOR TRIGGER ---
        else if (mg_strcmp(hm->uri, mg_str("/api/trigger")) == 0) {
            char id_buf[10] = {0};
            mg_http_get_var(&hm->query, "id", id_buf, sizeof(id_buf));
            int id = atoi(id_buf);
            if (id >= 0 && id < 100) {
                // Toggle the pin: if 1 (Secure), set to 0 (Violated)
                mock_gpio_pins[id] = (mock_gpio_pins[id] == 1) ? 0 : 1;
                printf("[MOCK] Zone %d is now %s\n", id, mock_gpio_pins[id] == 0 ? "VIOLATED" : "SECURE");
            }
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"ok\"}");
        }
        else {
            struct mg_http_serve_opts opts = {.root_dir = "."}; 
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

int main(void) {
    mg_log_set(0);
    storage_load_all(); 
    
    // --- FORCE STARTUP DISARMED & AUDIT ---
    engine_init(); 
    printf("\n=== STARTUP DIAGNOSTIC AUDIT ===\n");
    printf("Initial System State: %d (Forced Disarmed)\n", engine_get_arm_state());
    for(int i=0; i < z_count; i++) {
        printf(" Zone %d [%-10s]: %s\n", zones[i].id, zones[i].name, (digitalRead(zones[i].gpio)==0) ? "!! OPEN !!" : "SECURE");
    }
    printf("================================\n\n");

    monitor_init();
    mg_mgr_init(&mgr);
// 0.0.0.0 means "Listen to everything: WiFi, Ethernet, and Local"

    mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL);
    while (1) {
        mg_mgr_poll(&mgr, 10);
        engine_tick();
        usleep(100000);
    }
    return 0;
}