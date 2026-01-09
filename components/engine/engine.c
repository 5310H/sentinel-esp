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
    printf("[ENGINE] System Logic Initialized. Mode: DISARMED\n");
}

// --- STATE GETTER ---
int engine_get_arm_state(void) {
    return s_arm_state;
}

// --- KEYPAD AUTHENTICATION (Fixed Loop Logic) ---
keypad_result_t engine_check_keypad(const char* input_pin) {
    keypad_result_t res = {false, false, "Unknown"};

    if (!input_pin || strlen(input_pin) == 0) return res;

    printf("[AUTH] Verifying PIN: [%s]\n", input_pin);

    // 1. Check Master System PIN (from config.json)
    if (strcmp(input_pin, config.pin) == 0) {
        res.authenticated = true;
        res.is_admin = true;
        strcpy(res.name, "Master");
        printf("[AUTH] Success: Master User authenticated.\n");
        return res; 
    } 

    // 2. Iterate through User Array (from users.json)
    // We check every record up to u_count
    for (int i = 0; i < u_count; i++) {
        if (strcmp(input_pin, users[i].pin) == 0) {
            res.authenticated = true;
            res.is_admin = users[i].is_admin; 
            strncpy(res.name, users[i].name, sizeof(res.name) - 1);
            
            printf("[AUTH] Success: User '%s' found (Admin: %s)\n", 
                    res.name, res.is_admin ? "YES" : "NO");
            return res; // Stop at first match
        }
    }

    printf("[AUTH] Failed: No match for PIN [%s]\n", input_pin);
    return res; 
}

// --- LOCAL HARDWARE ACTION ---
void trigger_local_hardware(zone_t *z) {
    printf("[ALARM] Triggering hardware for zone: %s\n", z->name);
    
    // Activate Alarm Relays
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
    // 1. Timer Countdown (Exit/Entry Delays)
    if (s_timer > 0) {
        s_timer--;
        if (s_timer % 5 == 0 || s_timer < 5) {
            printf("[TICK] State: %d, Time Left: %ds\n", s_arm_state, s_timer);
        }
    }

    // 2. Chime Reset Timer
    if (s_chime_timer > 0) {
        s_chime_timer--;
        if (s_chime_timer == 0 && s_arm_state == 0) {
            for (int r = 0; r < r_count; r++) {
                if (strcmp(relays[r].type, "chime") == 0) __wrap_hal_set_relay(relays[r].id, false);
            }
        }
    }

    // 3. State Transitions
    if (s_arm_state == 1 && s_timer == 0) {
        s_arm_state = 2; // Exit Delay -> Armed
        printf("[STATE] -> 2 (ARMED) Mode: %d\n", s_current_mode);
    }
    
    if (s_arm_state == 3 && s_timer == 0) {
        printf("[TIMEOUT] Entry delay expired. Triggering full alarm!\n");
        for (int i = 0; i < z_count; i++) {
            if (is_zone_tripped(i)) engine_trigger_alarm(i);
        }
    }

    // 4. Zone Monitoring Loop
    for (int i = 0; i < z_count; i++) {
        bool is_tripped = is_zone_tripped(i);
        if (!is_tripped) continue;

        // A. 24-HOUR ZONES (Fire/Panic/Water)
        if (!zones[i].is_alarm_on_armed_only) {
//            printf("[CRITICAL] 24HR Zone Tripped: %s\n", zones[i].name);
            engine_trigger_alarm(i);
            continue; 
        }

        // B. CHIME (Only when disarmed)
        if (s_arm_state == 0 && zones[i].is_chime && s_chime_timer == 0) {
            printf("[EVENT] Chime: %s\n", zones[i].name);
            for (int r = 0; r < r_count; r++) {
                if (strcmp(relays[r].type, "chime") == 0) __wrap_hal_set_relay(relays[r].id, true);
            }
            s_chime_timer = 30; // 3 seconds at 10hz
        }
        
        // C. SMART ARMING LOGIC
        if (s_arm_state == 2) { 
            bool monitor = false;
            if (s_current_mode == ARM_AWAY) monitor = true;
            else if (s_current_mode == ARM_STAY && zones[i].is_perimeter) monitor = true;
            else if (s_current_mode == ARM_NIGHT && (zones[i].is_perimeter || !zones[i].is_interior)) monitor = true;

            if (monitor) {
                if (zones[i].is_perimeter) {
                    s_arm_state = 3; 
                    s_timer = config.entry_delay;
                    printf("[STATE] -> 3 (ENTRY DELAY) Triggered by: %s\n", zones[i].name);
                } else {
                    printf("[STATE] Immediate Alarm! Interior violation: %s\n", zones[i].name);
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
        s_arm_state = 1; // Start Exit Delay
        s_timer = config.exit_delay;
        printf("[UI] Arming Sequence Started. Mode: %d, Exit Delay: %ds\n", mode, s_timer);
    }
}

void engine_ui_disarm(void) {
    printf("[UI] System Disarmed via Keypad.\n");
    s_arm_state = 0;
    s_timer = 0;
    s_chime_timer = 0;
    __wrap_hal_set_siren(false);
    
    // Reset alert flags and turn off all relays
    for (int i = 0; i < z_count; i++) zones[i].is_alert_sent = false; 
    for (int r = 0; r < r_count; r++) __wrap_hal_set_relay(relays[r].id, false);
}