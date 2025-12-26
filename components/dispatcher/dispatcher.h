#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "config.h"

// Lifecycle
void dispatcher_init(Owner_t *owner);

// Main Actions
void dispatcher_send_alarm(Zone_t *zone);
void dispatcher_send_cancel(void);
void dispatcher_update_mqtt(ArmState_t state, AlarmStatus_t status);

#endif