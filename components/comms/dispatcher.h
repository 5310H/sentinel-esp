#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "storage_mgr.h"

/**
 * @brief Main entry point for the Engine to report a zone violation.
 * Handles routing to Noonlight, SMTP, and (eventually) Telegram.
 */
void dispatcher_alert(zone_t *z);

/**
 * @brief Called when the system is successfully disarmed.
 * Sends cancellation/stand-down notices to remote platforms.
 */
void dispatcher_cancel_alert(void);

#endif // DISPATCHER_H