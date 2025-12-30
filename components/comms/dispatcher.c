#include "dispatcher.h"
#include "smtp.h"
#include "noonlight.h"
#include <stdio.h>

extern owner_t owner;
extern user_t users[MAX_USERS];
extern int u_count;
extern zone_t zones[MAX_ZONES];
extern int z_count;

void notifier_init(void) {
    printf("[DISPATCHER] Initializing notification channels...\n");
}

void notifier_send(int zone_id) {
    zone_t *z = NULL;
    for(int i=0; i<z_count; i++) {
        if(zones[i].id == zone_id) {
            z = &zones[i];
            break;
        }
    }
    
    if (!z) return;

    printf("[DISPATCHER] ALERT! Triggering notifications for Zone: %s\n", z->name);

    // Call SMTP logic (Correctly passing 4 arguments)
    smtp_alert_all_contacts(&owner, users, u_count, z);

    // Call Noonlight logic (Now passing all 3 required arguments)
    noonlight_update_contacts(&owner, users, u_count);
}

void notifier_cancel_alarm(void) {
    printf("[DISPATCHER] Alarm cancelled. Sending updates...\n");
    smtp_send_cancellation(&owner, users, u_count);
}
