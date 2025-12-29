#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "storage_mgr.h" // This provides owner_t, user_t, and zone_t

void dispatcher_init(owner_t *owner, zone_t *zones, int z_count, user_t *users, int u_count);
void dispatcher_process_event(int event_type, int zone_id);

// Bridge for engine.c
void notifier_init(owner_t *o, zone_t *z, int zc, user_t *u, int uc);
void notifier_send(int event, int zone);
void notifier_cancel_alarm();

#endif