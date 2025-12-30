#ifndef NOONLIGHT_H
#define NOONLIGHT_H

#include "storage_mgr.h"

// Lifecycle Functions
void noonlight_create_alarm(owner_t *o, zone_t *z);
void noonlight_update_contacts(owner_t *o, user_t *users, int user_count);
void noonlight_update_events(const char *event_msg);
void noonlight_cancel_alarm(owner_t *o, const char *entered_pin);

// High-level bridge for the Dispatcher
void noonlight_send_event(owner_t *o, const char *msg, zone_t *z);

#endif