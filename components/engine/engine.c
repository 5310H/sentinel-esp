#include "engine.h"
#include "hal.h"
#include "notifier.h"
#include <stdio.h>
#include <string.h>

static int s_arm_state = 0;    // 0: Disarmed, 1: Away, 2: Stay
static int s_alarm_status = 0; // 0: OK, 2: Tripped

void engine_init(void) {
    s_arm_state = 0;
    s_alarm_status = 0;
    printf("[ENGINE] Core logic initialized.\n");
}

void engine_arm(int mode) {
    s_arm_state = mode;
    s_alarm_status = 0;
    printf("[ENGINE] System ARMED (Mode %d)\n", mode);
}

void engine_disarm(const char* pin) {
    // In a production system, you'd validate the PIN against User_t records
    s_arm_state = 0;
    s_alarm_status = 0;
    notifier_cancel_alarm();
    printf("[ENGINE] System DISARMED\n");
}

void engine_tick(void) {
    // Periodic logic: handle entry/exit delay timers here
}

int engine_get_arm_state(void) {
    return s_arm_state;
}

int engine_get_status(void) {
    return s_alarm_status;
}

void engine_trigger_alarm(void) {
    s_alarm_status = 2; // Tripped
    printf("[ENGINE] ALARM STATE ACTIVATED!\n");
}

// Logic: Check the hardware layer to see if the relay is currently energized
bool engine_get_relay_state(int relay_id) {
    return hal_get_relay_state(relay_id);
}