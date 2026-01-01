#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include "storage_mgr.h"
#include "mongoose.h"
#include "engine.h"

// Globals from your component objects
extern zone_t zones[MAX_ZONES];
extern int z_count;
extern relay_t relays[MAX_RELAYS]; 
extern int r_count;
extern owner_t owner;             
extern user_t users[MAX_USERS];
extern int u_count;

static bool s_running = true;
struct mg_mgr mgr;

// Mock hardware state tracking
static bool mock_zone_violated[MAX_ZONES] = {false};
static bool mock_relay_state[MAX_RELAYS] = {false};
static int last_engine_state = -1;

void handle_sigint(int sig) { s_running = false; }

// --- HAL WRAPPERS ---
void __wrap_hal_set_relay(int id, bool state) {
    static bool last_states[MAX_RELAYS] = {0};
    for(int i = 0; i < r_count; i++) {
        if(relays[i].id == id) {
            mock_relay_state[i] = state;
            if (last_states[i] != state) {
                printf("\n>>> [HAL] RELAY %d -> %s <<<\n", id, state ? "ON (ALARM LATCHED)" : "OFF");
                last_states[i] = state;
                fflush(stdout);
            }
            break;
        }
    }
}

bool __wrap_hal_get_zone_state(int id) {
    if (id > 0 && id <= MAX_ZONES) return mock_zone_violated[id - 1];
    return false;
}

void __wrap_hal_set_siren(bool state) {
    static bool last_siren = false;
    if (state != last_siren) {
        printf("\n[HAL] SIREN -> %s\n", state ? "ON" : "OFF");
        last_siren = state;
    }
}

void __wrap_hal_set_strobe(bool state) {
    static bool last_strobe = false;
    if (state != last_strobe) {
        printf("\n[HAL] STROBE -> %s\n", state ? "ON" : "OFF");
        last_strobe = state;
    }
}

bool __wrap_hal_get_relay_state(int id) {
    for(int i = 0; i < r_count; i++) {
        if(relays[i].id == id) return mock_relay_state[i];
    }
    return false;
}

// --- WEB API HANDLER ---
static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        const char *headers = "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n";

        if (mg_match(hm->uri, mg_str("/api/trigger"), NULL)) {
            char id_str[10];
            if (mg_http_get_var(&hm->query, "id", id_str, sizeof(id_str)) > 0) {
                int z_id = atoi(id_str);
                if (z_id > 0 && z_id <= z_count) {
                    printf("\n[WEB] TRIP: Zone %d (%s)\n", z_id, zones[z_id-1].name);
                    mock_zone_violated[z_id - 1] = true;
                }
            }
            mg_http_reply(c, 200, headers, "{\"status\":\"ok\"}");
        } else if (mg_match(hm->uri, mg_str("/api/arm"), NULL)) {
            engine_arm(1); 
            mg_http_reply(c, 200, headers, "{\"status\":\"success\"}");
        } else if (mg_match(hm->uri, mg_str("/api/disarm"), NULL)) {
            engine_arm(0); 
            for(int i=0; i<MAX_ZONES; i++) mock_zone_violated[i] = false;
            mg_http_reply(c, 200, headers, "{\"status\":\"success\"}");
        } else if (mg_match(hm->uri, mg_str("/api/config"), NULL)) {
            char z_list[2048] = "";
            for (int i = 0; i < z_count; i++) {
                char tmp[256];
                snprintf(tmp, sizeof(tmp), "{\"id\":%d,\"name\":\"%s\",\"type\":\"%s\",\"violated\":%s}",
                    i+1, zones[i].name, zones[i].type, mock_zone_violated[i] ? "true" : "false");
                strcat(z_list, tmp);
                if (i < z_count - 1) strcat(z_list, ",");
            }
            char r_list[1024] = "";
            for (int i = 0; i < r_count; i++) {
                char tmp[256];
                snprintf(tmp, sizeof(tmp), "{\"ID\":%d,\"Name\":\"Relay %d\",\"state\":%s}",
                    relays[i].id, relays[i].id, mock_relay_state[i] ? "true" : "false");
                strcat(r_list, tmp);
                if (i < r_count - 1) strcat(r_list, ",");
            }
            int cur_state = engine_get_arm_state();
            mg_http_reply(c, 200, headers,
                "{\"state\":\"%s\",\"zones\":[%s],\"relays\":[%s],\"owner\":\"%s\"}",
                (cur_state > 0) ? "ARMED" : "DISARMED", z_list, r_list, "MOCK_MODE");
        } else {
            struct mg_http_serve_opts opts = { .root_dir = "." };
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

int main(void) {
    mg_log_set(MG_LL_NONE); 
    signal(SIGINT, handle_sigint);

    storage_load_all(&owner, zones, &z_count, users, &u_count, relays, &r_count);
    
    // HARD SILENCE: Force notification systems to stay quiet during testing
    u_count = 0; 
    memset(owner.email, 0, sizeof(owner.email));

    engine_init();
    mg_mgr_init(&mgr);
    
    if (mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL) == NULL) {
        printf("Error: Port 8000 busy\n");
        return 1;
    }

    printf("\n[SENTINEL MOCK] ALARM SYSTEM RUNNING\n");
    printf("Notifications: DISABLED (MOCK MODE)\n");
    printf("Web Dashboard: http://localhost:8000\n\n");

    static int hold_timers[MAX_ZONES] = {0};

    while (s_running) {
        mg_mgr_poll(&mgr, 1); 
        engine_tick();         

        // Momentary Sensor Logic (Zone resets, Relay latches)
        for(int i = 0; i < z_count; i++) {
            if (mock_zone_violated[i]) {
                if (++hold_timers[i] > 10) { 
                    mock_zone_violated[i] = false;
                    hold_timers[i] = 0;
                    printf("[MOCK] Sensor %d back to normal.\n", i+1);
                }
            }
        }

        int cur_state = engine_get_arm_state();
        if (cur_state != last_engine_state) {
            printf("[ENGINE] Mode Change: %s\n", (cur_state > 0) ? "ARMED" : "DISARMED");
            last_engine_state = cur_state;
        }
        usleep(50000); 
    }
    
    mg_mgr_free(&mgr);
    return 0;
}