#include "monitor.h"
#include "notifier.h"
#include "engine.h"
#include <string.h>
#include <stdio.h>

void monitor_init(owner_t *_owner, user_t *_users, int *_u_count) {
    // notifier_init is now void, so we call it without arguments
    notifier_init();
    printf("[MONITOR] System monitor initialized.\n");
}

void monitor_process_event(const char *event_type, int zone_id) {
    if (event_type == NULL) return;

    if (strcmp(event_type, "alarm") == 0) {
        printf("[MONITOR] Processing Alarm Event for Zone %d\n", zone_id);
        
        // Use our new simplified notifier logic
        notifier_send(zone_id);
        
        // Trigger the physical engine alarm
        engine_trigger_alarm();
    }
}
