#include "engine.h"
#include "hal.h"
#include "notifier.h"
#include "hal.h"
#include "storage_mgr.h"
#include <stdio.h>
#include <string.h>

// External data from storage_mgr
extern relay_t relays[MAX_RELAYS];
extern int r_count;
extern zone_t zones[MAX_ZONES];
extern int z_count;

// Internal state
static int s_arm_state = 0;    // 0: Disarmed, 1: Armed
static int s_alarm_timer = 0;   // Ticks remaining for Alarm relays
static int s_chime_timer = 0;   // Ticks remaining for Chime relays

// Duration settings (adjust based on your tick rate)
#define ALARM_DURATION 200 // ~20 seconds at 100ms ticks
#define CHIME_DURATION 30  // ~3 seconds at 100ms ticks

void engine_init(void) {
    s_arm_state = 0;
    s_alarm_timer = 0;
    s_chime_timer = 0;
    printf("[ENGINE] Core initialized. Rules: Fire (Always), Police (Armed), Chime (Disarmed).\n");
}

void engine_arm(int mode) {
    s_arm_state = mode;
    printf("[ENGINE] System state changed to: %s\n", mode > 0 ? "ARMED" : "DISARMED");
}

int engine_get_arm_state(void) {
    return s_arm_state;
}

// Internal helper to fire relays based on their type (alarm/chime)
static void set_relays_by_type(const char* target_type, bool state) {
    for (int i = 0; i < r_count; i++) {
        if (strcmp(relays[i].type, target_type) == 0) {
            hal_set_relay(relays[i].id, state);
        }
    }
}

void engine_tick(void) {
    // 1. Check Zones and Apply Rules
    for (int i = 0; i < z_count; i++) {
        // hal_get_zone_state in mock.c handles the "one-shot" reset
        if (hal_get_zone_state(zones[i].id)) {
            
            // RULE: Type=fire -> Trigger all 'alarm' relays + notification (Always)
            if (strcmp(zones[i].type, "fire") == 0) {
                printf("[RULE] FIRE ZONE %d TRIPPED!\n", zones[i].id);
                set_relays_by_type("alarm", true);
                s_alarm_timer = ALARM_DURATION; // Start/Reset timer
                notifier_send(zones[i].id); 
            }
            
            // RULE: Type=police -> Trigger all 'alarm' relays + notification (Armed only)
            else if (strcmp(zones[i].type, "police") == 0 && s_arm_state > 0) {
                printf("[RULE] POLICE ZONE %d TRIPPED (ARMED)!\n", zones[i].id);
                set_relays_by_type("alarm", true);
                s_alarm_timer = ALARM_DURATION; // Start/Reset timer
                notifier_send(zones[i].id);
            }
            
            // RULE: Type=chime -> Trigger all 'chime' relays (Disarmed only)
            else if (strcmp(zones[i].type, "chime") == 0 && s_arm_state == 0) {
                printf("[RULE] CHIME ZONE %d TRIPPED (DISARMED)!\n", zones[i].id);
                set_relays_by_type("chime", true);
                s_chime_timer = CHIME_DURATION; // Start/Reset timer
            }
        }
    }

    // 2. Handle Alarm Duration Timer
    if (s_alarm_timer > 0) {
        s_alarm_timer--;
        if (s_alarm_timer == 0) {
            printf("[ENGINE] Alarm duration expired. Turning off alarm relays.\n");
            set_relays_by_type("alarm", false);
        }
    }

    // 3. Handle Chime Duration Timer
    if (s_chime_timer > 0) {
        s_chime_timer--;
        if (s_chime_timer == 0) {
            printf("[ENGINE] Chime duration expired. Turning off chime relays.\n");
            set_relays_by_type("chime", false);
        }
    }
}

void engine_trigger_alarm(void) {
    printf("[ENGINE] Manual alarm trigger received!\n");
    for (int i = 0; i < r_count; i++) {
        if (strcmp(relays[i].type, "alarm") == 0) {
            hal_set_relay(relays[i].id, 1);
        }
    }
}
