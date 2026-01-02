#ifndef NOONLIGHT_H
#define NOONLIGHT_H

#include "storage_mgr.h"

// --- GLOBALS ---
// Stores the ID returned by Noonlight to allow for event logging and cancellation
extern char active_alarm_id[STR_SMALL];

// --- CORE API POSTS ---

/**
 * 1. Creates the initial alarm.
 * Sets the active_alarm_id on success.
 */
bool noonlight_create_alarm(config_t *conf, zone_t *z);

/**
 * 2. Logs specific zone triggers to the alarm timeline.
 */
void noonlight_log_event(config_t *conf, zone_t *z);

/**
 * 3. Sends dispatch instructions (Gate codes, pet info, etc).
 */
void noonlight_send_instructions(config_t *conf);

/**
 * 4. Syncs the backup contacts (user_t array) to the dispatcher.
 */
void noonlight_sync_people(config_t *conf, user_t *users, int count);

// --- ALARM CONTROL ---

/**
 * Updates the alarm status to "canceled".
 * Requires the PIN of the person who canceled it.
 */
bool noonlight_cancel_alarm(config_t *conf, const char *pin);

#endif