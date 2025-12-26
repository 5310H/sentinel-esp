#ifndef MONITOR_H
#define MONITOR_H

#include "config.h"

void monitor_init(Owner_t* owner, User_t* users, int u_count);
void monitor_process_event(int zone_id, const char* zone_name);

#endif