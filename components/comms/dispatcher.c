#include <stdio.h>
#include <string.h>
#include "storage_mgr.h"
#include "dispatcher.h"
#include "noonlight.h"
#include "smtp.h"

static owner_t *system_owner;
static zone_t *system_zones;

void notifier_init(owner_t *o, zone_t *z, int zc, user_t *u, int uc) {
    system_owner = o;
    system_zones = z;
    printf("[DISPATCHER] Initialized. Mode: %s\n", o->notify);
}

void notifier_send(int event, int zone_idx) {
    if (strcmp(system_owner->notify, "service") == 0) {
        // Live Noonlight API Call
        noonlight_send_event(system_owner, "ALARM_ID_LINUX", &system_zones[zone_idx]);
    } else {
        // Live SMTP Email Call
        char body[512];
        snprintf(body, sizeof(body), "SENTINEL ALARM: Zone %d (%s) triggered.", 
                 zone_idx, system_zones[zone_idx].name);
        smtp_send_email(system_owner, "CRITICAL: Home Alarm", body);
    }
}

void notifier_cancel_alarm() {
    if (strcmp(system_owner->notify, "service") == 0) {
        noonlight_cancel_alarm(system_owner, 0, "1234");
    } else {
        smtp_send_email(system_owner, "SENTINEL: Status", "System Disarmed/Canceled.");
    }
}