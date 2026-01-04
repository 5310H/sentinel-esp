/**
 * @file smtp.h
 * @brief Function prototypes for the libcurl-based SMTP notification service.
 */

#ifndef SMTP_H
#define SMTP_H

#include "storage_mgr.h"

/**
 * @brief Triggers a mass email alert to the system config and all authorized users.
 * * @param config      Pointer to the config configuration (server, credentials, config email).
 * @param users      Array of secondary users to notify.
 * @param user_count Number of secondary users in the array.
 * @param z          The specific zone that triggered the alert.
 */
void smtp_alert_all_contacts(config_t *config, user_t *users, int user_count, zone_t *z);

/**
 * @brief Sends a cancellation/disarm notice to the config and all authorized users.
 * * @param config      Pointer to the config configuration.
 * @param users      Array of secondary users to notify.
 * @param user_count Number of secondary users in the array.
 */
void smtp_send_cancellation(config_t *config, user_t *users, int user_count);

/**
 * @brief Legacy/General purpose function for sending a single custom email.
 * * @param config   Pointer to the config configuration.
 * @param subject The email subject line.
 * @param body    The email body text.
 */
void smtp_send_email(config_t *config, const char *subject, const char *body);

#endif // SMTP_H