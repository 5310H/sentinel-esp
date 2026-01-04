#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "engine.h"
#include "storage_mgr.h"
#include "dispatcher.h"
#include "monitor.h"
#include "mongoose.h"

// FIX: Global mock flag for dispatcher
int is_mock = 1; 

//static int last_engine_state = -1;
// Initialize all pins to false (not tripped)
static bool mock_gpio_pins[100] = {false}; 
struct mg_mgr mgr;

// --- HAL MOCKS ---
// Returns 0 (LOW/Tripped) or 1 (HIGH/OK)
int digitalRead(int pin) { 
    if (pin < 0 || pin >= 100) return 1;
    return mock_gpio_pins[pin] ? 0 : 1; 
}

void __wrap_hal_set_siren(bool state) { 
    printf("\n>>> [HAL] SIREN: %s <<<\n", state ? "ON" : "OFF"); 
}

void __wrap_hal_set_relay(int id, bool state) { 
    printf("[HAL] Relay %d: %s\n", id, state ? "ACTIVE" : "INACTIVE"); 
}

void __wrap_hal_set_strobe(bool state) { }
int __wrap_hal_get_zone_state(int gpio) { return digitalRead(gpio); }
int __wrap_hal_get_relay_state(int id) { return 0; }

// --- WEB HANDLER ---
static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;

        // 1. API: STATUS / CONFIG
        if (mg_strcmp(hm->uri, mg_str("/api/config")) == 0 || mg_strcmp(hm->uri, mg_str("/api/status")) == 0) {
            int state = engine_get_arm_state();
            static char buf[8192];
            memset(buf, 0, sizeof(buf));

            int len = snprintf(buf, sizeof(buf), 
                "{\"success\":true,\"state\":%d,\"config\":\"%s\",\"zones\":[", 
                state, (config.name[0] != '\0') ? config.name : "Test config");

            for (int i = 0; i < z_count; i++) {
                int is_violated = (digitalRead(zones[i].gpio) == 0);
                if (i > 0) len += snprintf(buf + len, sizeof(buf) - len, ",");
                
                len += snprintf(buf + len, sizeof(buf) - len, 
                    "{\"id\":%d,\"name\":\"%s\",\"type\":\"%s\",\"gpio\":%d,\"violated\":%s}", 
                    i + 1, 
                    (zones[i].name[0] != '\0') ? zones[i].name : "Unnamed", 
                    (zones[i].type[0] != '\0') ? zones[i].type : "police", 
                    zones[i].gpio,
                    is_violated ? "true" : "false");
            }

            len += snprintf(buf + len, sizeof(buf) - len, "],\"relays\":[");
            for (int i = 0; i < r_count; i++) {
                if (i > 0) len += snprintf(buf + len, sizeof(buf) - len, ",");
                len += snprintf(buf + len, sizeof(buf) - len,
                    "{\"id\":%d,\"name\":\"%s\",\"state\":%d}", 
                    relays[i].id, 
                    (relays[i].name[0] != '\0') ? relays[i].name : "Relay", 0);
            }

            strcat(buf, "]}");
            mg_http_reply(c, 200, "Content-type: application/json\r\nAccess-Control-Allow-Origin: *\r\n", "%s", buf);
        }
        // 2. API: ARM
        else if (mg_strcmp(hm->uri, mg_str("/api/arm")) == 0) {
            engine_arm(1);
            mg_http_reply(c, 200, "Content-type: application/json\r\nAccess-Control-Allow-Origin: *\r\n", "{\"success\":true}");
        }
        // 3. API: DISARM
        else if (mg_strcmp(hm->uri, mg_str("/api/disarm")) == 0) {
            engine_disarm(config.pin);
            mg_http_reply(c, 200, "Content-type: application/json\r\nAccess-Control-Allow-Origin: *\r\n", "{\"success\":true}");
        }
        // 4. API: TRIGGER (The critical logic)
        else if (mg_strcmp(hm->uri, mg_str("/api/trigger")) == 0) {
            char id_str[10] = {0};
            mg_http_get_var(&hm->query, "id", id_str, sizeof(id_str));
            int id = atoi(id_str);
            
            if (id > 0 && id <= z_count) {
                int gpio = zones[id-1].gpio;
                // Toggle the mock state
                mock_gpio_pins[gpio] = !mock_gpio_pins[gpio];
                printf("[MOCK] Request Zone %d -> GPIO %d is now %s\n", 
                        id, gpio, mock_gpio_pins[gpio] ? "LOW (Tripped)" : "HIGH (OK)");
            }
            mg_http_reply(c, 200, "Content-type: application/json\r\nAccess-Control-Allow-Origin: *\r\n", "{\"success\":true}");
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
    storage_debug_print();

    // CRITICAL DEBUG: Print GPIO mappings to terminal on boot
    printf("[DEBUG] Zone Mapping Check:\n");
    for(int i=0; i<z_count; i++) {
        printf("  Zone %d: %s on GPIO %d\n", i+1, zones[i].name, zones[i].gpio);
    }

    engine_init();
    monitor_init();

    mg_mgr_init(&mgr);
    if (mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL) == NULL) {
        printf("Failed to listen\n");
        return 1;
    }

    printf("[SYSTEM] Dashboard live at http://localhost:8000\n");

    while (1) {
        mg_mgr_poll(&mgr, 10); 
        engine_tick();
        usleep(10000); 
    }
    return 0;
}