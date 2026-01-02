#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "engine.h"
#include "storage_mgr.h"
#include "dispatcher.h"

// --- Global State Variables ---
int s_arm_state = 0;      // 0: Disarmed, 1: Arming, 2: Armed, 3: Entry Delay, 4: Alarmed
int s_timer = 0;          // Seconds remaining for Entry/Exit delays
int s_chime_timer = 0;    // Pulse/Debounce for chimes

extern int digitalRead(int pin);
extern void __wrap_hal_set_relay(int id, bool state);
extern void __wrap_hal_set_siren(bool state);

// --- LOCAL HARDWARE ACTION ---
// This replaces the old scattered relay/siren calls.
// It ensures local protection is active BEFORE the network call starts.
void trigger_local_hardware(zone_t *z) {
    printf("[LOCAL] !!! ALARM !!! %s triggered. Engaging hardware...\n", z->name);
    
    // 1. Fire the specific relay tied to this zone (from JSON)
    if (z->relay_id > 0) {
        __wrap_hal_set_relay(z->relay_id, true);
    }
    
    // 2. Fire the Global Siren
    __wrap_hal_set_siren(true);
}

// --- ALARM PROCESSOR ---
void process_alarm_event(int index) {
    if (index < 0 || index >= z_count) return;

    if (!zones[index].alert_sent) {
        // STEP 1: Local Functions Completed First
        trigger_local_hardware(&zones[index]);
        
        // STEP 2: Hand off to the Dispatcher
        // The Engine is now done; the Dispatcher decides if it goes to Noonlight, SMTP, etc.
        dispatcher_alert(&zones[index]); 
        
        zones[index].alert_sent = true; 
    }
}

// --- SYSTEM-WIDE TRIGGER ---
void engine_trigger_alarm(void) {
    if (s_arm_state != 4) { 
        s_arm_state = 4;
        printf("[ENGINE] State Change -> ALARMED (Global Latch Engaged)\n");

        // Identify which zone(s) caused the latch
        for (int i = 0; i < z_count; i++) {
            if (digitalRead(zones[i].gpio) == 0) {
                process_alarm_event(i);
            }
        }
    }
}

// --- STATE MACHINE & TICK LOGIC ---
void engine_tick(void) {
    // 1. Monitor All Zones
    for (int i = 0; i < z_count; i++) {
        bool is_tripped = (digitalRead(zones[i].gpio) == 0);

        if (is_tripped) {
            // A. CHIME LOGIC (Only in Disarmed state)
            if (zones[i].chime && s_arm_state == 0 && s_chime_timer == 0) {
                printf("[EVENT] Chime: %s\n", zones[i].name);
                if (zones[i].relay_id > 0) __wrap_hal_set_relay(zones[i].relay_id, true);
                s_chime_timer = 50; 
            }
            
            // B. 24-HOUR ZONES (Smoke/CO/Panic) - Triggers regardless of Arm State
            if (!zones[i].alarm_on_armed_only) {
                engine_trigger_alarm();
            }
            
            // C. INTRUSION ZONES (Only if fully ARMED)
            if (s_arm_state == 2) { 
                if (zones[i].is_perimeter) {
                    printf("[ENGINE] Perimeter Breach: %s. Entry Delay started.\n", zones[i].name);
                    s_arm_state = 3; 
                    s_timer = config.EntryDelay;
                } else if (zones[i].is_interior) {
                    engine_trigger_alarm();
                }
            }
        }
    }

    // 2. Process Timers and State Transitions
    if (s_timer > 0) s_timer--;
    if (s_chime_timer > 0) s_chime_timer--;

    switch (s_arm_state) {
        case 1: // ARMING (Exit Delay)
            if (s_timer == 0) {
                s_arm_state = 2; // ARMED
                printf("[ENGINE] System ARMED.\n");
            }
            break;

        case 3: // ENTRY DELAY
            if (s_timer == 0) {
                engine_trigger_alarm(); // Time's up
            }
            break;

        default: break;
    }
}

// --- PUBLIC COMMANDS ---

void engine_init(void) {
    s_arm_state = 0;
    printf("[ENGINE] Core initialized. Mapping %d zones to HAL.\n", z_count);
}

void engine_arm(int state) {
    if (state == 1) { // Request to Arm
        printf("[ENGINE] Arming... Starting Exit Delay (%ds).\n", config.ExitDelay);
        s_arm_state = 1;
        s_timer = config.ExitDelay;
    }
}

void engine_disarm(const char* pin) {
    if (strcmp(pin, config.pin) == 0) {
        printf("[ENGINE] PIN Verified. Disarming.\n");
        
        // If disarming from an active alarm, notify Dispatcher to cancel remote services
        if (s_arm_state == 4) {
            dispatcher_cancel_alert();
        }

        s_arm_state = 0;
        s_timer = 0;
        
        // Local Hardware Reset
        __wrap_hal_set_siren(false);
        for (int i = 0; i < z_count; i++) {
            zones[i].alert_sent = false;
            if (zones[i].relay_id > 0) __wrap_hal_set_relay(zones[i].relay_id, false);
        }
    } else {
        printf("[ENGINE] Invalid PIN entered.\n");
    }
}

int engine_get_arm_state(void) {
    return s_arm_state;
}