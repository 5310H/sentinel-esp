#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "storage_mgr.h"

void notifier_init(void);
void notifier_send(int zone_id);
void notifier_cancel_alarm(void);

#endif
