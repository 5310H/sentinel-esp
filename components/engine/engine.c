#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "engine.h"
#include "storage_mgr.h"
#include "dispatcher.h"

// --- Global State Variables ---
int s_arm_state = 0;      // 0: Disarmed, 1: Exit, 2: Armed, 3: Entry, 4: Alarmed
int s_timer = 0;          
int s_chime_timer = 0;    
arm_mode_t s_current_mode = ARM_AWAY;
// Prototype for the HAL function (usually in hal.h, but adding it here fixes the warning)
int hal_get_zone_state(int gpio);
// --- NEW: Violation Tracking for UI ---
char s_violation_name[64] = "None";
char s_violation_type[32] = "None";

// Hardware Mocks (Linked via main_mock.o)
extern int digitalRead(int pin);
extern void __wrap_hal_set_relay(int id, bool state);
extern void __wrap_hal_set_siren(bool state);

// --- HARDWARE ABSTRACTION ---
static bool is_zone_tripped(int index) {
    if (index < 0 || index >= z_count) return false;
    // I2C zones not yet implemented in mock
    if (zones[index].is_i2c) return false; 
    return (digitalRead(zones[index].gpio) == 0);
}

// --- INITIALIZATION ---
void engine_init(void) {
    s_arm_state = 0;
    s_timer = 0;
    s_chime_timer = 0;
    s_current_mode = ARM_AWAY;
    strcpy(s_violation_name, "None");
    strcpy(s_violation_type, "None");
    printf("[ENGINE] System Logic Initialized. Mode: DISARMED\n");
}

// --- STATE GETTERS ---
int engine_get_arm_state(void) {
    return s_arm_state;
}

// Getters for main_mock.c to use in JSON output
const char* engine_get_violation_name(void) { return s_violation_name; }
const char* engine_get_violation_type(void) { return s_violation_type; }

// --- KEYPAD AUTHENTICATION ---
keypad_result_t engine_check_keypad(const char* input_pin) {
    keypad_result_t res = {false, false, "Unknown"};
    if (!input_pin || strlen(input_pin) == 0) return res;

    if (strcmp(input_pin, config.pin) == 0) {
        res.authenticated = true;
        res.is_admin = true;
        strcpy(res.name, "Master");
        return res; 
    } 

    for (int i = 0; i < u_count; i++) {
        if (strcmp(input_pin, users[i].pin) == 0) {
            res.authenticated = true;
            res.is_admin = users[i].is_admin; 
            strncpy(res.name, users[i].name, sizeof(res.name) - 1);
            return res; 
        }
    }
    return res; 
}

// --- LOCAL HARDWARE ACTION ---
void trigger_local_hardware(zone_t *z) {
    printf("[ALARM] Triggering hardware for zone: %s\n", z->name);
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
    
    if (!zones[index].is_alert_sent) {
        printf("[ENGINE] ALERT! %s violated.\n", zones[index].name);
        
        // Capture Violation Details for UI/Email
        strncpy(s_violation_name, zones[index].name, sizeof(s_violation_name)-1);
        strncpy(s_violation_type, zones[index].type, sizeof(s_violation_type)-1);

        trigger_local_hardware(&zones[index]);
        dispatcher_alert(&zones[index]); 
        zones[index].is_alert_sent = true; 
    }
}

// --- EXTERNAL TRIGGER ---
void engine_trigger_alarm(int trigger_index) {
    if (s_arm_state != 4) { 
        s_arm_state = 4;
        printf("[STATE] -> 4 (ALARMED)\n");
    }
    process_alarm_event(trigger_index);
}

// --- CORE TICK LOGIC ---
void engine_tick(void) {
    if (s_timer > 0) {
        s_timer--;
        if (s_timer % 5 == 0 || s_timer < 5) {
            printf("[TICK] State: %d, Time Left: %ds\n", s_arm_state, s_timer);
        }
    }

    if (s_chime_timer > 0) {
        s_chime_timer--;
        if (s_chime_timer == 0 && s_arm_state == 0) {
            for (int r = 0; r < r_count; r++) {
                if (strcmp(relays[r].type, "chime") == 0) __wrap_hal_set_relay(relays[r].id, false);
            }
        }
    }

    if (s_arm_state == 1 && s_timer == 0) {
        s_arm_state = 2; // Exit -> Armed
        printf("[STATE] -> 2 (ARMED)\n");
    }
    
    if (s_arm_state == 3 && s_timer == 0) {
        printf("[TIMEOUT] Entry delay expired. Triggering full alarm!\n");
        for (int i = 0; i < z_count; i++) {
            if (is_zone_tripped(i)) engine_trigger_alarm(i);
        }
    }

    // Zone Monitoring Loop
    for (int i = 0; i < z_count; i++) {
        bool is_tripped = is_zone_tripped(i);
        if (!is_tripped) continue;

        // 24-HOUR ZONES (Fire/Panic)
        if (!zones[i].is_alarm_on_armed_only) {
            engine_trigger_alarm(i);
            continue; 
        }

        // CHIME
        if (s_arm_state == 0 && zones[i].is_chime && s_chime_timer == 0) {
            for (int r = 0; r < r_count; r++) {
                if (strcmp(relays[r].type, "chime") == 0) __wrap_hal_set_relay(relays[r].id, true);
            }
            s_chime_timer = 30;
        }
        
        // SMART ARMING
        if (s_arm_state == 2) { 
            bool monitor = false;
            if (s_current_mode == ARM_AWAY) monitor = true;
            else if (s_current_mode == ARM_STAY && zones[i].is_perimeter) monitor = true;
            else if (s_current_mode == ARM_NIGHT && (zones[i].is_perimeter || !zones[i].is_interior)) monitor = true;

            if (monitor) {
                // Capture details even during Entry Delay
                strncpy(s_violation_name, zones[i].name, sizeof(s_violation_name)-1);
                strncpy(s_violation_type, zones[i].type, sizeof(s_violation_type)-1);

                if (zones[i].is_perimeter) {
                    s_arm_state = 3; 
                    s_timer = config.entry_delay;
                    printf("[STATE] -> 3 (ENTRY DELAY) by: %s\n", zones[i].name);
                } else {
                    engine_trigger_alarm(i);
                }
            }
        }
    }
}

// --- PUBLIC UI COMMANDS ---
void engine_ui_arm(arm_mode_t mode) {
    if (s_arm_state == 0) { 
        s_current_mode = mode;
        s_arm_state = 1; 
        s_timer = config.exit_delay;
        // Clear old violations when arming
        strcpy(s_violation_name, "None");
        strcpy(s_violation_type, "None");
    }
}

void engine_ui_disarm(void) {
    printf("[UI] System Disarmed.\n");
    s_arm_state = 0;
    s_timer = 0;
    s_chime_timer = 0;
    
    // RESET VIOLATION INFO
    strcpy(s_violation_name, "None");
    strcpy(s_violation_type, "None");

    __wrap_hal_set_siren(false);
    for (int i = 0; i < z_count; i++) zones[i].is_alert_sent = false; 
    for (int r = 0; r < r_count; r++) __wrap_hal_set_relay(relays[r].id, false);
}
bool is_ready(void) {
    // Check all zones
    for (int i = 0; i < z_count; i++) {
        // We only care about zones that are NOT "arm-only" (like 24hr fire/smoke)
        // or interior zones that are bypassed.
        // If a perimeter door is open (digitalRead == 0), we are NOT ready.
        if (zones[i].is_perimeter && hal_get_zone_state(zones[i].gpio) == 0) {
            return false;
        }
    }
    return true;
}