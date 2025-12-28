#ifndef COMM_NOONLIGHT_H
#define COMM_NOONLIGHT_H

#include <stdbool.h>

/* Service Slot Indices */
#define SLOT_POLICE   0
#define SLOT_FIRE     1
#define SLOT_MEDICAL  2
#define SLOT_OTHER    3

struct owner {
    char *name; char *phone; char *pin;
    char *address1; char *address2; char *city; char *state; char *zip;
    double lat; double lng;
    bool monitor_police; bool monitor_fire; bool monitor_medical; bool monitor_other;
    
    /* Credentials from setup.json */
    char MonitorServiceID[64];   // The base URL (Sandbox/Prod)
    char MonitorServiceKey[128];  // The Bearer Token
};

struct zone {
    char *name; char *location; char *type;
    char *mfg; char *model; char *description;
};

struct contact {
    char *name; char *phone; char *pin; char *notify;
};

/* Public API */
void  noonlight_handle_trigger(struct owner *o, struct zone *z, struct contact *c, int c_count);
void  noonlight_sync_people(struct owner *o, const char *alarm_id, struct contact *c, int c_count);
void  noonlight_send_event(struct owner *o, const char *alarm_id, struct zone *z);
void  noonlight_cancel_alarm(struct owner *o, int slot, const char *entered_pin);
char* noonlight_get_active_id(int slot);

#endif