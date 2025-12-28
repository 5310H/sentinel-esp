#include "mongoose.h"
#include "storage.h"
#include "engine.h"
#include "hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool simulated_zones[MAX_ZONES];

bool __wrap_hal_get_zone_state(int zone_id) {
    if (zone_id >= 0 && zone_id < MAX_ZONES) return simulated_zones[zone_id];
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

        // 1. LOGIN
        if (mg_match(hm->uri, mg_str("/api/login"), NULL)) {
            char pin[16] = {0};
            mg_http_get_var(&hm->body, "pin", pin, sizeof(pin));
            for (int i = 0; i < u_count; i++) {
                if (strcmp(users[i].PIN, pin) == 0) {
                    mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                        "{\"id\":%d,\"name\":\"%s\",\"isAdmin\":%s}",
                        users[i].ID, users[i].Name, users[i].IsAdmin ? "true" : "false");
                    return;
                }
            }
            mg_http_reply(c, 401, "", "{\"error\":\"Invalid PIN\"}");
        }
        // 2. ARMING
        else if (mg_match(hm->uri, mg_str("/api/arm"), NULL)) {
            char state_str[10] = {0};
            mg_http_get_var(&hm->body, "state", state_str, sizeof(state_str));
            engine_arm(atoi(state_str)); 
            log_event("USER", "ARM REQUEST");
            mg_http_reply(c, 200, "", "{\"status\":\"OK\"}");
        }
        // 3. DISARMING (FIXED: Added pin argument)
        else if (mg_match(hm->uri, mg_str("/api/disarm"), NULL)) {
            char pin[16] = {0};
            mg_http_get_var(&hm->body, "pin", pin, sizeof(pin));
            engine_disarm(pin);
            log_event("USER", "DISARM ATTEMPT");
            mg_http_reply(c, 200, "", "{\"status\":\"OK\"}");
        }
        // 4. PANIC
        else if (mg_match(hm->uri, mg_str("/api/panic"), NULL)) {
            engine_trigger_alarm();
            log_event("ALARM", "PANIC TRIGGERED");
            mg_http_reply(c, 200, "", "{\"status\":\"OK\"}");
        }
        // 5. ZONE TRIGGER
        else if (mg_match(hm->uri, mg_str("/api/zone/trigger"), NULL)) {
            char id_str[10] = {0}, state_str[10] = {0};
            mg_http_get_var(&hm->body, "id", id_str, sizeof(id_str));
            mg_http_get_var(&hm->body, "tripped", state_str, sizeof(state_str));
            int zid = atoi(id_str);
            if (zid >= 0 && zid < MAX_ZONES) {
                simulated_zones[zid] = (strcmp(state_str, "true") == 0);
            }
            mg_http_reply(c, 200, "", "{\"status\":\"OK\"}");
        }
        // 6. STATUS
        else if (mg_match(hm->uri, mg_str("/api/config"), NULL)) {
            char json[8192] = {0}, z_buf[2048] = "[", r_buf[1024] = "[";
            for (int i=0; i<z_count; i++) {
                char b[256]; snprintf(b, sizeof(b), "{\"id\":%d,\"name\":\"%s\",\"tripped\":%s}%s", 
                    zones[i].ID, zones[i].Name, simulated_zones[zones[i].ID]?"true":"false", (i<z_count-1)?",":"");
                strcat(z_buf, b);
            }
            strcat(z_buf, "]");
            for (int i=0; i<r_count; i++) {
                char b[256]; snprintf(b, sizeof(b), "{\"id\":%d,\"active\":%s}%s", 
                    relays[i].ID, engine_get_relay_state(relays[i].ID)?"true":"false", (i<r_count-1)?",":"");
                strcat(r_buf, b);
            }
            strcat(r_buf, "]");
            snprintf(json, sizeof(json), "{\"arm_state\":%d,\"zones\":%s,\"relays\":%s}", 
                     engine_get_arm_state(), z_buf, r_buf);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json);
        }
        // 7. HISTORY (FIXED: Correct mg_file_read usage)
        else if (mg_match(hm->uri, mg_str("/api/users/add"), NULL)) {
            char name[32] = {0}, pin[16] = {0};
            mg_http_get_var(&hm->body, "name", name, sizeof(name));
            mg_http_get_var(&hm->body, "pin", pin, sizeof(pin));
            if (u_count < MAX_USERS) {
                strncpy(users[u_count].Name, name, sizeof(users[u_count].Name));
                strncpy(users[u_count].PIN, pin, sizeof(users[u_count].PIN));
                users[u_count].ID = u_count + 1;
                users[u_count].IsAdmin = false;
                u_count++;
                storage_save_all(&owner, users, u_count, zones, z_count, relays, r_count, rules, rule_count);
                log_event("ADMIN", "Added new user");
                mg_http_reply(c, 200, "", "{\"status\":\"OK\"}");
            } else {
                mg_http_reply(c, 400, "", "{\"error\":\"User limit reached\"}");
            }
        }
        else if (mg_match(hm->uri, mg_str("/api/history"), NULL)) {
            struct mg_str s = mg_file_read(&mg_fs_posix, "history.csv");
            if (s.buf != NULL) {
                mg_http_reply(c, 200, "Content-Type: text/csv\r\nContent-Disposition: attachment; filename=\"history_backup.csv\"\r\n", "%.*s", (int) s.len, s.buf);
                free((void *) s.buf);
            } else {
                mg_http_reply(c, 200, "", "No logs found.");
            }
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
    printf("Sentinel Console Active: %s\n", s_http_addr);
    for (;;) {
        engine_tick();
        mg_mgr_poll(&mgr, 50);
    }
    return 0;
}
