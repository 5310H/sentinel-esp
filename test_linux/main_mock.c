#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include "storage_mgr.h"
#include "mongoose.h"
#include "engine.h"

// Globals from component objects
extern zone_t zones[MAX_ZONES];
extern int z_count;
extern relay_t relays[MAX_RELAYS]; 
extern int r_count;
extern config_t config;             
extern user_t users[MAX_USERS];
extern int u_count;

static bool s_running = true;
struct mg_mgr mgr;

static bool mock_zone_violated[MAX_ZONES] = {false};
static bool mock_relay_state[MAX_RELAYS] = {false};
static int last_engine_state = -1;

void handle_sigint(int sig) { s_running = false; }

// --- HAL WRAPPERS ---
int digitalRead(int pin) {
    for(int i = 0; i < z_count; i++) {
        if(zones[i].gpio == pin) return mock_zone_violated[i] ? 0 : 1;
    }
    return 1;
}

void __wrap_hal_set_relay(int id, bool state) {
    for(int i = 0; i < r_count; i++) {
        if(relays[i].id == id) {
            mock_relay_state[i] = state;
            printf("\n>>> [HAL] RELAY %d (%s) -> %s <<<\n", id, relays[i].name, state ? "ON" : "OFF");
        }
    }
}
// Add this to your mock file so engine.o can find it
void __wrap_hal_set_siren(bool state) {
    printf("\n[MOCK HAL] SIREN state changed to: %s\n", state ? "ON" : "OFF");
}

// Add these to main_mock.c to satisfy monitor.o
void notifier_init(void) {
    // printf("[MOCK] Notifier initialized (Redirected to Dispatcher logic)\n");
}

void notifier_send(void *data) {
    // If you want monitor to actually trigger alerts:
    // dispatcher_alert((zone_t *)data);
    printf("[MOCK] Notifier send called.\n");
}

void __wrap_hal_set_strobe(bool state) {
    printf("[MOCK HAL] STROBE state changed to: %s\n", state ? "ON" : "OFF");
}
// --- WEB API HANDLER ---
static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        const char *headers = "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n";

        // TRIGGER ZONE
        if (mg_match(hm->uri, mg_str("/api/trigger"), NULL)) {
            char id_str[10];
            if (mg_http_get_var(&hm->query, "id", id_str, sizeof(id_str)) > 0) {
                int z_id = atoi(id_str);
                for(int i=0; i<z_count; i++) {
                    if(zones[i].id == z_id) {
                        mock_zone_violated[i] = true;
                        printf("[API] Triggered: %s\n", zones[i].name);
                    }
                }
            }
            mg_http_reply(c, 200, headers, "{\"status\":\"ok\"}");
        } 
        // ARM SYSTEM
        else if (mg_match(hm->uri, mg_str("/api/arm"), NULL)) {
            printf("[API] Arming System...\n");
            engine_arm(1); 
            mg_http_reply(c, 200, headers, "{\"status\":\"success\"}");
        } 
        // DISARM SYSTEM
        else if (mg_match(hm->uri, mg_str("/api/disarm"), NULL)) {
            printf("[API] Disarming System...\n");
            engine_disarm(config.pin); 
            for(int i=0; i<MAX_ZONES; i++) mock_zone_violated[i] = false;
            mg_http_reply(c, 200, headers, "{\"status\":\"success\"}");
        } 
        // CONFIG SYNC
        else if (mg_match(hm->uri, mg_str("/api/config"), NULL)) {
            char z_list[4096] = "", r_list[2048] = "";
            for (int i = 0; i < z_count; i++) {
                char tmp[512];
                snprintf(tmp, sizeof(tmp), "{\"ID\":%d,\"Name\":\"%s\",\"Type\":\"%s\",\"violated\":%s}",
                    zones[i].id, zones[i].name, zones[i].type, mock_zone_violated[i] ? "true" : "false");
                strcat(z_list, tmp);
                if (i < z_count - 1) strcat(z_list, ",");
            }
            for (int i = 0; i < r_count; i++) {
                char tmp[512];
                snprintf(tmp, sizeof(tmp), "{\"ID\":%d,\"Name\":\"%s\",\"state\":%s}",
                    relays[i].id, relays[i].name, mock_relay_state[i] ? "true" : "false");
                strcat(r_list, tmp);
                if (i < r_count - 1) strcat(r_list, ",");
            }
            int cur_state = engine_get_arm_state();
            const char* states[] = {"DISARMED", "ARMING", "ARMED", "ENTRY_DELAY", "ALARMED"};
            mg_http_reply(c, 200, headers, "{\"state\":\"%s\",\"zones\":[%s],\"relays\":[%s],\"owner\":\"%s\"}",
                states[cur_state], z_list, r_list, config.name);
        } else {
            struct mg_http_serve_opts opts = { .root_dir = "." };
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

int main(void) {
    mg_log_set(MG_LL_NONE); 
    signal(SIGINT, handle_sigint);
    storage_load_all();
    engine_init();
    mg_mgr_init(&mgr);
    if (mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL) == NULL) return 1;
    
    printf("\n[SENTINEL MOCK] ALARM SYSTEM RUNNING\n");

    while (s_running) {
        mg_mgr_poll(&mgr, 50);
        engine_tick();
        for(int i = 0; i < z_count; i++) {
            if (mock_zone_violated[i]) {
                static int timers[MAX_ZONES] = {0};
                if (++timers[i] > 40) { mock_zone_violated[i] = false; timers[i] = 0; }
            }
        }
        usleep(10000); 
    }
    mg_mgr_free(&mgr);
    return 0;
}