#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "engine.h"
#include "storage_mgr.h"
#include "dispatcher.h"

// --- Global State Variables ---
int s_arm_state = 0;      
int s_timer = 0;          
int s_chime_timer = 0;    

extern int digitalRead(int pin);
extern void __wrap_hal_set_relay(int id, bool state);
extern void __wrap_hal_set_siren(bool state);

// --- HARDWARE ABSTRACTION ---
// This handles both standard GPIO and the new I2C logic from your JSON
static bool is_zone_tripped(int index) {
    if (zones[index].isi2c) {
        // Future: Add I2C bus read logic here using zones[index].i2caddress
        // For now, it returns false (not tripped) to prevent ghost alarms
        return false; 
    }
    return (digitalRead(zones[index].gpio) == 0);
}

// --- LOCAL HARDWARE ACTION ---
void trigger_local_hardware(zone_t *z) {
    printf("[LOCAL] !!! ALARM !!! %s (Type: %s) triggered.\n", z->name, z->type);
    
    // Decoupled Relay Lookup: Find all relays of type "alarm"
    for (int r = 0; r < r_count; r++) {
        if (strcmp(relays[r].type, "alarm") == 0) {
            __wrap_hal_set_relay(relays[r].id, true);
        }
    }
    __wrap_hal_set_siren(true);
}

// --- ALARM PROCESSOR ---
void process_alarm_event(int index) {
    if (index < 0 || index >= z_count) return;

    // Gatekeeper: Only alert once per zone per alarm cycle
    if (!zones[index].alert_sent) {
        printf("[ENGINE] Dispatching alert for Zone %d: %s\n", index + 1, zones[index].name);
        
        trigger_local_hardware(&zones[index]);
        dispatcher_alert(&zones[index]); 
        
        zones[index].alert_sent = true; 
    }
}

// --- SYSTEM-WIDE TRIGGER ---
void engine_trigger_alarm(int trigger_index) {
    if (s_arm_state != 4) { 
        s_arm_state = 4;
        printf("[STATE] -> ALARMED (Initial Trigger: %s)\n", zones[trigger_index].name);
    }
    process_alarm_event(trigger_index);
}

// --- CORE TICK LOGIC ---
void engine_tick(void) {
    // 1. Timers
    if (s_timer > 0) s_timer--;
    
    if (s_chime_timer > 0) {
        s_chime_timer--;
        if (s_chime_timer == 0 && s_arm_state == 0) {
            // Find chime relays and turn them off when timer expires
            for (int r = 0; r < r_count; r++) {
                if (strcmp(relays[r].type, "chime") == 0) {
                    __wrap_hal_set_relay(relays[r].id, false);
                }
            }
        }
    }

    // 2. Monitor Zones Individually
    for (int i = 0; i < z_count; i++) {
        bool is_tripped = is_zone_tripped(i);

        if (is_tripped) {
            // A. 24-HOUR ZONES (Fire/Smoke/Panic)
            if (!zones[i].isalarmonarmedonly) {
                engine_trigger_alarm(i);
                continue; 
            }

            // B. CHIME (Only if Disarmed)
            if (s_arm_state == 0 && zones[i].ischime && s_chime_timer == 0) {
                printf("[EVENT] Chime: %s\n", zones[i].name);
                for (int r = 0; r < r_count; r++) {
                    if (strcmp(relays[r].type, "chime") == 0) {
                        __wrap_hal_set_relay(relays[r].id, true);
                    }
                }
                s_chime_timer = 50; 
            }
            
            // C. INTRUSION (Only if Armed)
            if (s_arm_state == 2) { 
                if (zones[i].isperimeter) {
                    s_arm_state = 3; 
                    s_timer = config.entrydelay;
                    printf("[ENGINE] Entry Delay Started by %s\n", zones[i].name);
                } else if (zones[i].isinterior) {
                    engine_trigger_alarm(i);
                }
            }
        }
    }

    // 3. State Transitions
    if (s_arm_state == 1 && s_timer == 0) {
        s_arm_state = 2; 
        printf("[STATE] -> SYSTEM ARMED\n");
    }

    if (s_arm_state == 3 && s_timer == 0) {
        for (int i = 0; i < z_count; i++) {
            if (is_zone_tripped(i)) engine_trigger_alarm(i);
        }
    }
}

// --- PUBLIC COMMANDS ---

void engine_init(void) {
    s_arm_state = 0;
    s_timer = 0;
    s_chime_timer = 0;
    printf("[ENGINE] Logic initialized.\n");
}

void engine_arm(int state) {
    if (s_arm_state == 0) { 
        s_arm_state = 1;
        s_timer = config.exitdelay;
        printf("[ENGINE] Arming... Exit Delay: %ds\n", config.exitdelay);
    }
}

void engine_disarm(const char* pin) {
    if (strcmp(pin, config.pin) == 0) {
        printf("[ENGINE] DISARMED. Resetting state.\n");
        s_arm_state = 0;
        s_timer = 0;
        __wrap_hal_set_siren(false);
        
        for (int i = 0; i < z_count; i++) {
            zones[i].alert_sent = false; 
        }

        // Global Reset: Turn off all relays on disarm
        for (int r = 0; r < r_count; r++) {
            __wrap_hal_set_relay(relays[r].id, false);
        }
    }
}

int engine_get_arm_state(void) { return s_arm_state; }