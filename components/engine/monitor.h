#ifndef MONITOR_H
#define MONITOR_H

#include "storage_mgr.h"

void monitor_init(owner_t *_owner, user_t *_users, int *_u_count);
void monitor_process_event(const char *event_type, int zone_id);

#endif
