#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "storage_mgr.h"
#include "noonlight.h"

// Static buffer to hold the Alarm ID returned by the Noonlight API
static char active_alarm_id[128] = {0};

// Helper function to handle the standard CURL setup for Noonlight
static CURL* setup_noonlight_curl(struct curl_slist **headers) {
    CURL *curl = curl_easy_init();
    if (curl) {
        *headers = curl_slist_append(*headers, "Content-Type: application/json");
        // In production, you would add your API Key here:
        // *headers = curl_slist_append(*headers, "Authorization: Bearer YOUR_TOKEN");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headers);
    }
    return curl;
}

/**
 * 1. CREATE ALARM
 */
void noonlight_create_alarm(owner_t *o, zone_t *z) {
    CURL *curl;
    struct curl_slist *headers = NULL;
    curl = setup_noonlight_curl(&headers);

    if (curl) {
        char data[512];
        // Note: Noonlight expects a specific JSON body to initiate dispatch
        snprintf(data, sizeof(data), 
            "{\"source\": \"Sentinel-ESP\", \"location\": {\"address\": \"Home\"}, \"services\": {\"police\": true}}");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.noonlight.com/dispatch/v1/alarms");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        printf("[NOONLIGHT] API POST: Creating alarm for %s...\n", z->name);
        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            // LIVE LOGIC: In a real integration, we'd parse the JSON response here 
            // to extract the "id" field. For this simulator, we generate a mock tracking ID.
            snprintf(active_alarm_id, sizeof(active_alarm_id), "ALARM_%ld", (long)time(NULL));
            printf("[NOONLIGHT] API SUCCESS: Alarm ID %s saved.\n", active_alarm_id);
        } else {
            fprintf(stderr, "[NOONLIGHT] API ERROR: %s\n", curl_easy_strerror(res));
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

/**
 * 2. UPDATE CONTACTS (Instructions)
 */
void noonlight_update_contacts(owner_t *o, user_t *users, int user_count) {
    if (strlen(active_alarm_id) == 0) return;

    for (int i = 0; i < user_count; i++) {
        CURL *curl;
        struct curl_slist *headers = NULL;
        curl = setup_noonlight_curl(&headers);

        if (curl) {
            char url[256];
            char data[512];
            // Endpoint to add instructions/contacts to a specific alarm
            snprintf(url, sizeof(url), "https://api.noonlight.com/dispatch/v1/alarms/%s/instructions", active_alarm_id);
            snprintf(data, sizeof(data), "{\"type\": \"contact\", \"name\": \"%s\", \"phone\": \"%s\"}", 
                     users[i].name, users[i].phone);

            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

            printf("[NOONLIGHT] API POST: Syncing contact %s...\n", users[i].name);
            curl_easy_perform(curl);

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
    }
}

/**
 * 3. UPDATE EVENTS (Timeline)
 */
void noonlight_update_events(const char *event_msg) {
    if (strlen(active_alarm_id) == 0) return;

    CURL *curl;
    struct curl_slist *headers = NULL;
    curl = setup_noonlight_curl(&headers);

    if (curl) {
        char url[256];
        char data[512];
        snprintf(url, sizeof(url), "https://api.noonlight.com/dispatch/v1/alarms/%s/events", active_alarm_id);
        snprintf(data, sizeof(data), "{\"event_type\": \"dispatched\", \"details\": \"%s\"}", event_msg);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        printf("[NOONLIGHT] API POST: Logging event: %s\n", event_msg);
        curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

/**
 * 4. CANCEL ALARM
 */
void noonlight_cancel_alarm(owner_t *o, const char *entered_pin) {
    if (strlen(active_alarm_id) == 0) return;

    CURL *curl;
    struct curl_slist *headers = NULL;
    curl = setup_noonlight_curl(&headers);

    if (curl) {
        char url[256];
        // Cancellation is typically a DELETE or a PATCH to status
        snprintf(url, sizeof(url), "https://api.noonlight.com/dispatch/v1/alarms/%s/status", active_alarm_id);
        
        // We simulate the cancel by changing status to 'canceled'
        char data[128];
        snprintf(data, sizeof(data), "{\"status\": \"canceled\", \"pin\": \"%s\"}", entered_pin);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        printf("[NOONLIGHT] API PATCH: Canceling Alarm %s with PIN %s\n", active_alarm_id, entered_pin);
        curl_easy_perform(curl);

        // Clear the ID so we know we are no longer in an active alarm state
        memset(active_alarm_id, 0, sizeof(active_alarm_id));

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}