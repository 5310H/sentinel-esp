/**
 * @file smtp.c
 * @brief Handles automated email alerts using libcurl over SMTPS.
 */

#include "storage_mgr.h"
#include "smtp.h"
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

/**
 * @brief Internal helper to execute a single SMTP transaction.
 * * This function handles the low-level curl configuration, including:
 * - SMTPS handshake and SSL/TLS requirement.
 * - Credential authentication.
 * - Payload formatting using fmemopen to simulate a file stream.
 * * @param config Pointer to system config containing server config and credentials.
 * @param target_email The recipient's email address.
 * @param subject The email subject line.
 * @param body The main message content.
 */
static void smtp_execute_send(config_t *config, const char *target_email, const char *subject, const char *body) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    struct curl_slist *recipients = NULL;
    char url[256];
    
    // Construct SMTPS URL (e.g., smtps://smtp.gmail.com:465)
    snprintf(url, sizeof(url), "smtps://%s:%d", config->smtp_server, config->smtp_port);

    // Set server and authentication details
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERNAME, config->smtp_user);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, config->smtp_pass);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, config->smtp_user);

    // Add recipient to the envelope
    recipients = curl_slist_append(recipients, target_email);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    // Format the raw SMTP payload with headers
    char payload[2048];
    snprintf(payload, sizeof(payload),
             "To: %s\r\nFrom: %s\r\nSubject: %s\r\n\r\n%s",
             target_email, config->smtp_user, subject, body);

    // Provide payload to curl via a memory stream (fmemopen)
    FILE *temp = fmemopen((void*)payload, strlen(payload), "r");
    curl_easy_setopt(curl, CURLOPT_READDATA, temp);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    printf("[SMTP] Attempting delivery to: %s\n", target_email);
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) 
        fprintf(stderr, "[SMTP] Error: %s\n", curl_easy_strerror(res));
    else 
        printf("[SMTP] Success: Message delivered.\n");

    // Cleanup resources
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
    fclose(temp);
}

/**
 * @brief Orchestrates a mass-alert to the config and all secondary users.
 * * Called by the dispatcher when a zone breach is detected.
 */
void smtp_alert_all_contacts(config_t *config, user_t *users, int user_count, zone_t *z) {
    char subject[256];
    char body[512];
    snprintf(subject, sizeof(subject), "CRITICAL: Sentinel Alarm - %s", z->name);
    snprintf(body, sizeof(body), "Breach detected in %s. Local authorities or Noonlight may be notified.", z->name);

    // First, notify the config
    smtp_execute_send(config, config->email, subject, body);

    // Second, loop through the authorized user list
    for (int i = 0; i < user_count; i++) {
        smtp_execute_send(config, users[i].email, subject, body);
    }
}

/**
 * @brief Notifies all parties that the alarm state has ended.
 */
void smtp_send_cancellation(config_t *config, user_t *users, int user_count) {
    const char *subject = "Sentinel Status: Alarm Cancelled";
    const char *body = "The emergency alarm has been disarmed and cancelled.";

    smtp_execute_send(config, config->email, subject, body);
    for (int i = 0; i < user_count; i++) {
        smtp_execute_send(config, users[i].email, subject, body);
    }
}