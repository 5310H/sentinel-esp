#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "mongoose.h"
#include "engine.h"
#include "storage_mgr.h"

// --- PROTOTYPES ---
void monitor_init(void); // Silences the implicit declaration warning

// --- GLOBAL VARIABLES ---
static struct mg_mgr mgr;
int mock_gpio_pins[100] = { [0 ... 99] = 1 }; 

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

// --- MOCK HAL WRAPPERS ---
// Matches your -Wl,--wrap flags in the compiler command

void __wrap_hal_set_relay(int relay_id, bool active) { }
bool __wrap_hal_get_relay_state(int relay_id) { return false; }
void __wrap_hal_set_siren(bool active) { }
void __wrap_hal_set_strobe(bool active) { }
int __wrap_hal_get_zone_state(int zone_id) { return 1; } // Default normal

// --- WEB HANDLER ---
static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        
        // 1. USER MGMT
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
        else if (mg_strcmp(hm->uri, mg_str("/api/users/delete")) == 0) {
            char name[STR_SMALL] = {0};
            get_json_str(hm->body, "$.name", name, sizeof(name));
            user_drop(users, &u_count, name);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"ok\"}");
        }

        // 2. AUTH & CONTROL
        else if (mg_strcmp(hm->uri, mg_str("/api/auth")) == 0) {
            char pin_in[STR_SMALL] = {0};
            get_json_str(hm->body, "$.pin", pin_in, sizeof(pin_in));
            keypad_result_t res = engine_check_keypad(pin_in);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                "{\"authenticated\":%s,\"is_admin\":%s,\"name\":\"%s\",\"state\":%d}",
                res.authenticated ? "true" : "false", res.is_admin ? "true" : "false",
                res.name, engine_get_arm_state());
        }
        else if (mg_strcmp(hm->uri, mg_str("/api/arm")) == 0) {
            bool can_arm = true;
            for (int i = 0; i < z_count; i++) {
                if (zones[i].is_perimeter && digitalRead(zones[i].gpio) == 0) {
                    can_arm = false;
                    break;
                }
            }
            if (!can_arm) {
                mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"success\":false,\"msg\":\"Perimeter Open\"}");
            } else {
                char m_str[20] = {0};
                get_json_str(hm->body, "$.mode", m_str, sizeof(m_str));
                arm_mode_t mode = (strcmp(m_str, "stay") == 0) ? ARM_STAY : (strcmp(m_str, "night") == 0) ? ARM_NIGHT : ARM_AWAY;
                engine_ui_arm(mode);
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"success\":true}");
            }
        }
        else if (mg_strcmp(hm->uri, mg_str("/api/disarm")) == 0) {
            engine_ui_disarm();
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"success\":true}");
        }

        // 3. STATUS
        else if (mg_strcmp(hm->uri, mg_str("/api/status")) == 0) {
            bool is_ready = true;
            static char s_buf[10240];
            for (int i = 0; i < z_count; i++) {
                if (zones[i].is_perimeter && digitalRead(zones[i].gpio) == 0) { is_ready = false; break; }
            }
            int s_len = snprintf(s_buf, sizeof(s_buf), 
                               "{\"state\":%d,\"is_ready\":%s,\"siteName\":\"%s\",\"zones\":[", 
                               engine_get_arm_state(), is_ready ? "true" : "false", config.name);
            for (int i = 0; i < z_count; i++) {
                s_len += snprintf(s_buf + s_len, sizeof(s_buf) - s_len, 
                    "{\"id\":%d,\"name\":\"%s\",\"type\":\"%s\",\"violated\":%s,\"peri\":%s}%s", 
                    i + 1, zones[i].name, zones[i].type, 
                    (digitalRead(zones[i].gpio) == 0) ? "true" : "false",
                    zones[i].is_perimeter ? "true" : "false",
                    (i < z_count - 1) ? "," : "");
            }
            s_len += snprintf(s_buf + s_len, sizeof(s_buf) - s_len, "],\"relays\":[]}");
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", s_buf);
        }
        else if (mg_strcmp(hm->uri, mg_str("/api/trigger")) == 0) {
            char id_str[10] = {0};
            mg_http_get_var(&hm->query, "id", id_str, sizeof(id_str));
            int id = atoi(id_str);
            if (id > 0 && id <= z_count) {
                int gpio = zones[id-1].gpio;
                mock_gpio_pins[gpio] = !mock_gpio_pins[gpio];
            }
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"success\":true}");
        }
        else {
            struct mg_http_serve_opts opts = {.root_dir = "."}; 
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

int main(void) {
    mg_log_set(MG_LL_NONE); 
    storage_load_all(); 
    engine_init();
    monitor_init();
    
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL);

    while (1) {
        mg_mgr_poll(&mgr, 10);
        engine_tick();
        usleep(100000);
    }
    return 0;
}