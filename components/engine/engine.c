#include "engine.h"
#include "hal.h"
#include "notifier.h"
#include <stdio.h>
#include <string.h>

// External data from storage_mgr (via engine.h/storage_mgr.h)
extern relay_t relays[MAX_RELAYS];
extern int r_count;
extern zone_t zones[MAX_ZONES];
extern int z_count;

static int s_arm_state = 0;
static int s_alarm_timer = 0;
static int s_chime_timer = 0;

#define ALARM_DURATION 200 
#define CHIME_DURATION 30  

// Internal helper - static is okay here because it's only used in this file
static void set_relays_by_type(const char* target_type, bool state) {
    for (int i = 0; i < r_count; i++) {
        if (strcmp(relays[i].type, target_type) == 0) {
            hal_set_relay(relays[i].id, state);
        }
    }
}

void engine_init(void) {
    s_arm_state = 0;
    s_alarm_timer = 0;
    s_chime_timer = 0;
    for(int i = 0; i < z_count; i++) {
        zones[i].alert_sent = false;
    }
    printf("[ENGINE] Core initialized.\n");
}

void engine_arm(int mode) {
    s_arm_state = mode;
    if (mode == 0) {
        s_alarm_timer = 0;
        set_relays_by_type("alarm", false);
        for(int i = 0; i < z_count; i++) { 
            zones[i].alert_sent = false; 
        }
        notifier_cancel_alarm(); 
    }
    printf("[ENGINE] State changed to: %s\n", mode > 0 ? "ARMED" : "DISARMED");
}

// FIX: Added for main_mock.c
int engine_get_arm_state(void) {
    return s_arm_state;
}

// FIX: Added for monitor.c
void engine_trigger_alarm(void) {
    printf("[ENGINE] Manual panic trigger received!\n");
    set_relays_by_type("alarm", true);
    s_alarm_timer = ALARM_DURATION;
    // Notify dispatcher that a manual alarm occurred (using a dummy ID or 99)
    notifier_send(99); 
}

// FIX: Full implementation for main_mock.c
void engine_tick(void) {
    for (int i = 0; i < z_count; i++) {
        bool tripped = hal_get_zone_state(zones[i].id);

        if (tripped) {
            // FIRE Rule (Always)
            if (strcmp(zones[i].type, "fire") == 0) {
                set_relays_by_type("alarm", true);
                s_alarm_timer = ALARM_DURATION;
                if (!zones[i].alert_sent) {
                    notifier_send(zones[i].id);
                    zones[i].alert_sent = true;
                }
            }
            // POLICE Rule (Armed Only)
            else if (strcmp(zones[i].type, "police") == 0 && s_arm_state > 0) {
                set_relays_by_type("alarm", true);
                s_alarm_timer = ALARM_DURATION;
                if (!zones[i].alert_sent) {
                    notifier_send(zones[i].id);
                    zones[i].alert_sent = true;
                }
            }
            // CHIME Rule (Disarmed Only)
            else if (strcmp(zones[i].type, "chime") == 0 && s_arm_state == 0) {
                if (s_chime_timer == 0) {
                    set_relays_by_type("chime", true);
                    s_chime_timer = CHIME_DURATION;
                }
            }
        } else {
            // Reset notification flag when door closes
            zones[i].alert_sent = false;
        }
    }

    // Timer logic
    if (s_alarm_timer > 0 && --s_alarm_timer == 0) {
        printf("[ENGINE] Alarm timed out.\n");
        set_relays_by_type("alarm", false);
    }
    if (s_chime_timer > 0 && --s_chime_timer == 0) {
        set_relays_by_type("chime", false);
    }
}