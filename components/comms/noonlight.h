#ifndef NOONLIGHT_H
#define NOONLIGHT_H

#include "storage_mgr.h"

// Use owner_t and zone_t instead of "struct owner"
void noonlight_send_event(owner_t *o, const char *alarm_id, zone_t *z);
void noonlight_cancel_alarm(owner_t *o, int slot, const char *entered_pin);

#endif
