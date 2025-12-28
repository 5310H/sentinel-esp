#include <stdio.h>
#include <string.h>
#include "dispatcher.h"
void smtp_send_alert(owner_t *o, zone_t *z);

static owner_t *_owner;
static zone_t  *_zones;
static int      _zone_count;
static user_t   *_users;
static int      _user_count;

int current_state = STATE_DISARMED;

void dispatcher_init(owner_t *o, zone_t *z, int z_cnt, user_t *u, int u_cnt) {
    _owner = o;
    _zones = z;
    _zone_count = z_cnt;
    _users = u;
    _user_count = u_cnt;
    printf("[DISPATCHER] System Initialized. Monitoring %d zones for %s\n", _zone_count, _owner->name);
}

void dispatcher_process_event(int event_type, int zone_id) {
    printf("[DISPATCHER] Event %d received from Zone %d\n", event_type, zone_id);
    
    if (event_type == EVENT_ZONE_TRIP) {
        if (current_state != STATE_DISARMED) {
            printf("[ALARM] Triggering Noonlight and Notifications!\n");
            noonlight_trigger(_owner);
            // Assuming the first zone for the mock alert
            smtp_send_alert(_owner, &_zones[0]);
        } else {
            printf("[CHIME] Zone %d opened (Disarmed Mode)\n", zone_id);
        }
    }
}
