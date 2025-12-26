#include "mongoose.h"
#include "storage.h"
#include "engine.h"
#include "hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Forward declare existing HAL setters
extern void hal_set_relay_state(int relay_id, bool state);

// We define our own mock storage for zones since the HAL doesn't provide a setter
static bool simulated_zones[MAX_ZONES];

// We OVERRIDE the HAL getter to return our simulated values
// Note: If the linker complains about multiple definitions, 
// we will switch to direct memory access of the mock_relays/zones variable.
bool __wrap_hal_get_zone_state(int zone_id) {
    if (zone_id >= 0 && zone_id < MAX_ZONES) {
        return simulated_zones[zone_id];
    }
    return false;
}

static const char *s_http_addr = "http://0.0.0.0:8000";
static Owner_t owner;
static User_t users[MAX_USERS];
static Zone_t zones[MAX_ZONES];
static Relay_t relays[MAX_RELAYS];
static Rule_t rules[MAX_RULES];
static int u_count, z_count, r_count, rule_count;

void log_event(const char *user, const char *action) {
    FILE *f = fopen("history.csv", "a");
    if (f) {
        time_t now = time(NULL);
        char *ts = ctime(&now);
        if(ts) ts[strlen(ts) - 1] = '\0'; 
        fprintf(f, "%s | %-10s | %s\n", ts ? ts : "N/A", user, action);
        fclose(f);
    }
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        if (mg_match(hm->uri, mg_str("/api/login"), NULL)) {
            char pin[16] = {0};
            mg_http_get_var(&hm->body, "pin", pin, sizeof(pin));
            for (int i = 0; i < u_count; i++) {
                if (strcmp(users[i].PIN, pin) == 0) {
                    log_event(users[i].Name, "Login");
                    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                        "{\"id\":%d,\"name\":\"%s\",\"isAdmin\":%s}",
                        users[i].ID, users[i].Name, users[i].IsAdmin ? "true" : "false");
                    return;
                }
            }
            mg_http_reply(c, 401, "", "{\"error\":\"Invalid PIN\"}");
        }
        else if (mg_match(hm->uri, mg_str("/api/zone/trigger"), NULL)) {
            char id_str[10] = {0}, state_str[10] = {0};
            mg_http_get_var(&hm->body, "id", id_str, sizeof(id_str));
            mg_http_get_var(&hm->body, "tripped", state_str, sizeof(state_str));
            
            int zid = atoi(id_str);
            bool state = (strcmp(state_str, "true") == 0);
            
            if (zid >= 0 && zid < MAX_ZONES) {
                simulated_zones[zid] = state;
                log_event("Sim", state ? "Zone Tripped" : "Zone Cleared");
            }
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"OK\"}");
        }
        else if (mg_match(hm->uri, mg_str("/api/config"), NULL)) {
            char json[8192] = {0}, z_buf[2048] = "[", r_buf[1024] = "[";
            for (int i=0; i<z_count; i++) {
                char b[256]; snprintf(b, sizeof(b), "{\"id\":%d,\"name\":\"%s\",\"tripped\":%s}%s", 
                    zones[i].ID, zones[i].Name, hal_get_zone_state(zones[i].ID)?"true":"false", (i<z_count-1)?",":"");
                strcat(z_buf, b);
            }
            strcat(z_buf, "]");
            for (int i=0; i<r_count; i++) {
                char b[256]; snprintf(b, sizeof(b), "{\"id\":%d,\"name\":\"%s\",\"active\":%s}%s", 
                    relays[i].ID, relays[i].Name, hal_get_relay_state(relays[i].ID)?"true":"false", (i<r_count-1)?",":"");
                strcat(r_buf, b);
            }
            strcat(r_buf, "]");
            snprintf(json, sizeof(json), "{\"owner\":\"%s\",\"arm_state\":%d,\"zones\":%s,\"relays\":%s}", owner.Name, engine_get_arm_state(), z_buf, r_buf);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json);
        }
        else {
            struct mg_http_serve_opts opts = {.root_dir = "."};
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

int main() {
    storage_load_all(&owner, users, &u_count, zones, &z_count, relays, &r_count, rules, &rule_count);
    engine_init();
    struct mg_mgr mgr; mg_mgr_init(&mgr);
    mg_http_listen(&mgr, s_http_addr, fn, NULL);
    printf("Sentinel Mock Server running at %s\n", s_http_addr);
    for (;;) {
        engine_tick();
        mg_mgr_poll(&mgr, 50);
    }
    return 0;
}
