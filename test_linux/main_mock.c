#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "storage_mgr.h"
#include "mongoose.h"

// External functions from your engine
extern void engine_init(void);
extern void engine_tick(void);
extern void notifier_init(void); 

static bool s_running = true;
struct mg_mgr mgr;

void handle_sigint(int sig) { s_running = false; }

// --- THE WEB HANDLER ---
static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        const char *headers = "Content-Type: application/json\r\n"
                              "Access-Control-Allow-Origin: *\r\n";

        // Logic: Any request to /api/ returns the full system state
        if (hm->uri.len >= 5 && memcmp(hm->uri.buf, "/api/", 5) == 0) {
            
            // 1. Build Zones JSON string
            char z_list[2048] = "";
            for (int i = 0; i < z_count; i++) {
                char tmp[256];
                snprintf(tmp, sizeof(tmp), 
                    "{\"id\":%d,\"name\":\"%s\",\"type\":\"%s\",\"status\":\"secure\"}%s",
                    i+1, zones[i].name, zones[i].type, (i < z_count - 1 ? "," : ""));
                strcat(z_list, tmp);
            }

            // 2. Build Relays JSON string
            // We use .id and .type which we know exist. 
            // We generate the name string dynamically to avoid struct member errors.
            char r_list[1024] = "";
            for (int i = 0; i < r_count; i++) {
                char tmp[256];
                snprintf(tmp, sizeof(tmp), 
                    "{\"id\":%d,\"name\":\"Relay %d\",\"type\":\"%s\",\"state\":0,\"active\":false}%s",
                    relays[i].id, relays[i].id, relays[i].type, (i < r_count - 1 ? "," : ""));
                strcat(r_list, tmp);
            }

            // 3. Send the unified response
            mg_http_reply(c, 200, headers,
                "{"
                "\"status\":\"running\",\"armed\":0,\"alarm\":0,\"system_state\":\"READY\","
                "\"owner_email\":\"%s\",\"zone_count\":%d,\"relay_count\":%d,"
                "\"zones\":[%s],"
                "\"relays\":[%s],"
                "\"relay_list\":[%s],"
                "\"outputs\":[%s]"
                "}\n", 
                owner.email, z_count, r_count, z_list, r_list, r_list, r_list);
        }
        else {
            // Serve static web files (index.html, etc.) from the current directory
            struct mg_http_serve_opts opts = {.root_dir = "."};
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}

// --- HAL WRAPPERS (To satisfy the linker) ---
void __wrap_hal_set_relay(int relay_id, bool state) { 
    printf("\n[HAL] RELAY %d -> %s\n", relay_id, state ? "ON" : "OFF"); 
}
bool __wrap_hal_get_zone_state(int zone_id) { return false; }
void __wrap_hal_set_siren(bool state) { printf("\n[HAL] SIREN -> %s\n", state ? "ON" : "OFF"); }
void __wrap_hal_set_strobe(bool state) { printf("\n[HAL] STROBE -> %s\n", state ? "ON" : "OFF"); }
bool __wrap_hal_get_relay_state(int relay_id) { return false; }

// --- MAIN LOOP ---
int main(int argc, char *argv[]) {
    mg_log_set(MG_LL_NONE); 
    signal(SIGINT, handle_sigint);
    
    // Initialize storage and system
    storage_load_all(&owner, zones, &z_count, users, &u_count, relays, &r_count);
    notifier_init();
    
    mg_mgr_init(&mgr);
    if (mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL) == NULL) {
        printf("Failed to listen on port 8000\n");
        return 1;
    }
    
    printf("\n[MOCK] Sentinel Linux Active\n");
    printf("[MOCK] URL: http://localhost:8000\n\n");
    
    engine_init();

    while (s_running) {
        mg_mgr_poll(&mgr, 10); 
        engine_tick();
    }
    
    mg_mgr_free(&mgr);
    printf("\nShutting down...\n");
    return 0;
}