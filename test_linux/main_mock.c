#include "mongoose.h"
#include "storage.h"
#include "engine.h"
#include "hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *s_http_addr = "http://0.0.0.0:8000";
static Owner_t owner;
static User_t users[MAX_USERS];
static Zone_t zones[MAX_ZONES];
static Relay_t relays[MAX_RELAYS];
static Rule_t rules[MAX_RULES];
static int u_count, z_count, r_count, rule_count;

// --- HISTORY LOGGING ---
void log_event(const char *user, const char *action) {
    FILE *f = fopen("history.csv", "a");
    if (f) {
        time_t now = time(NULL);
        char *ts = ctime(&now);
        if(ts) ts[strlen(ts) - 1] = '\0'; 
        fprintf(f, "%s, %s, %s\n", ts ? ts : "N/A", user, action);
        fclose(f);
    }
}

// --- HELPERS ---
int find_user_index(int id) {
    for (int i = 0; i < u_count; i++) if (users[i].ID == id) return i;
    return -1;
}

bool is_admin(int id) {
    int idx = find_user_index(id);
    return (idx != -1 && users[idx].IsAdmin);
}

// --- MAIN HANDLER ---
static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        // 1. LOGIN
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
        // 2. DOWNLOAD HISTORY CSV
        else if (mg_match(hm->uri, mg_str("/api/history/download"), NULL)) {
            struct mg_http_serve_opts opts = {.root_dir = "."};
            mg_http_serve_file(c, hm, "history.csv", &opts);
        }
        // 3. RELAY TOGGLE (Using the corrected HAL function name)
        else if (mg_match(hm->uri, mg_str("/api/relay/toggle"), NULL)) {
            char id_str[10] = {0};
            mg_http_get_var(&hm->body, "id", id_str, sizeof(id_str));
            int rid = atoi(id_str);
            bool current = engine_get_relay_state(rid);
            
            // Fixed function name: hal_set_relay_state
            hal_set_relay_state(rid, !current); 
            
            char act[64];
            snprintf(act, sizeof(act), "Toggle Relay %d to %s", rid, !current ? "ON" : "OFF");
            log_event("WebUI", act);
            mg_http_reply(c, 200, "", "{\"status\":\"Toggled\"}");
        }
        // 4. CONFIG & USERS
        else if (mg_match(hm->uri, mg_str("/api/config"), NULL)) {
            char json[8192] = {0};
            char z_buf[2048] = "[", r_buf[1024] = "[";
            for (int i=0; i<z_count; i++) {
                char b[256]; snprintf(b, sizeof(b), "{\"id\":%d,\"name\":\"%s\"}%s", zones[i].ID, zones[i].Name, (i<z_count-1)?",":"");
                strcat(z_buf, b);
            }
            strcat(z_buf, "]");
            for (int i=0; i<r_count; i++) {
                char b[256]; 
                bool state = engine_get_relay_state(relays[i].ID);
                snprintf(b, sizeof(b), "{\"id\":%d,\"name\":\"%s\",\"active\":%s}%s", relays[i].ID, relays[i].Name, state?"true":"false", (i<r_count-1)?",":"");
                strcat(r_buf, b);
            }
            strcat(r_buf, "]");
            snprintf(json, sizeof(json), "{\"owner\":\"%s\",\"acct\":\"%s\",\"zones\":%s,\"relays\":%s}", 
                     owner.Name, owner.AccountID, z_buf, r_buf);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json);
        }
        else if (mg_match(hm->uri, mg_str("/api/users"), NULL)) {
            struct mg_str *hdr = mg_http_get_header(hm, "X-Admin-ID");
            int admin_id = (hdr && hdr->len > 0) ? atoi(hdr->buf) : -1;
            if (!is_admin(admin_id)) { mg_http_reply(c, 403, "", "{\"error\":\"Forbidden\"}"); return; }

            if (mg_strcmp(hm->method, mg_str("GET")) == 0) {
                char json[4096] = "[";
                for (int i = 0; i < u_count; i++) {
                    char u[256]; snprintf(u, sizeof(u), "{\"id\":%d,\"name\":\"%s\",\"isAdmin\":%s}%s",
                             users[i].ID, users[i].Name, users[i].IsAdmin ? "true":"false", (i < u_count-1)?",":"");
                    strcat(json, u);
                }
                strcat(json, "]");
                mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json);
            }
            else if (mg_strcmp(hm->method, mg_str("DELETE")) == 0) {
                char id_str[10] = {0};
                mg_http_get_var(&hm->body, "id", id_str, sizeof(id_str));
                int idx = find_user_index(atoi(id_str));
                if (idx >= 0) {
                    log_event("Admin", "Deleted User");
                    for (int i = idx; i < u_count - 1; i++) users[i] = users[i+1];
                    u_count--;
                    storage_save_all(&owner, users, u_count, zones, z_count, relays, r_count, rules, rule_count);
                    mg_http_reply(c, 200, "", "{\"status\":\"Deleted\"}");
                }
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
    struct mg_mgr mgr; mg_mgr_init(&mgr); mg_log_set(0);
    mg_http_listen(&mgr, s_http_addr, fn, NULL);
    printf("Sentinel Server Active at %s\n", s_http_addr);
    for (;;) { engine_tick(); mg_mgr_poll(&mgr, 50); }
    return 0;
}
