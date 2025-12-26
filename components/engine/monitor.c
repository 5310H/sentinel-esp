#include "monitor.h"
#include "notifier.h"
#include "engine.h"
#include <stdio.h>

static Owner_t* _owner;
static User_t* _users;
static int _u_count;

void monitor_init(Owner_t* owner, User_t* users, int u_count) {
    _owner = owner;
    _users = users;
    _u_count = u_count;
    notifier_init(_owner, _users, _u_count);
}

void monitor_process_event(int zone_id, const char* zone_name) {
    int state = engine_get_arm_state();
    
    // Log activity regardless of arm state
    printf("[MONITOR] Zone Activity: %s (ID: %d)\n", zone_name, zone_id);

    // If Armed (Any mode other than DISARMED), trigger alerts
    if (state != 0) { // 0 is ARMSTATE_DISARMED in your enum
        char msg[256];
        snprintf(msg, sizeof(msg), "SECURITY ALERT: %s triggered!", zone_name);
        
        notifier_send(NOTIFY_ALARM, msg);
        engine_trigger_alarm();
    }
}