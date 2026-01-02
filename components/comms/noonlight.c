#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "noonlight.h"
#include "cJSON.h"
#include "storage_mgr.h"

// The shared alarm ID used to link events/people/cancelations to the current incident
char active_alarm_id[STR_SMALL] = "";

// --- INTERNAL HELPER: CURL DATA CALLBACK ---
// This captures the response from Noonlight so we can extract the Alarm ID
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    char *response = (char *)userp;
    // Ensure we don't overflow the buffer (assuming 1024 bytes available)
    strncat(response, (char *)contents, realsize);
    return realsize;
}

// --- INTERNAL HELPER: SETUP CURL ---
static CURL* setup_noonlight_curl(config_t *conf, struct curl_slist **headers) {
    CURL *curl = curl_easy_init();
    if (curl) {
        char auth_header[512];
        *headers = curl_slist_append(*headers, "Content-Type: application/json");
        *headers = curl_slist_append(*headers, "Accept: application/json");

        // Use the MonitorServiceKey from your config as the Bearer Token
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", conf->MonitorServiceKey);
        *headers = curl_slist_append(*headers, auth_header);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headers);
    }
    return curl;
}

// --- 1. CREATE ALARM ---
bool noonlight_create_alarm(config_t *conf, zone_t *z) {
    CURL *curl;
    struct curl_slist *headers = NULL;
    char response_buffer[1024] = {0};
    bool success = false;

    curl = setup_noonlight_curl(conf, &headers);
    if (curl) {
        char data[1024];
        const char *police = (strcmp(z->type, "police") == 0) ? "true" : "false";
        const char *fire   = (strcmp(z->type, "fire") == 0) ? "true" : "false";

        snprintf(data, sizeof(data), 
            "{"
                "\"name\": \"%s\", \"phone\": \"%s\", \"originating_system_id\": \"%s\", "
                "\"location\": {"
                    "\"address\": \"%s\", \"city\": \"%s\", \"state\": \"%s\", \"zip\": \"%s\", "
                    "\"coordinates\": {\"lat\": %.6f, \"lng\": %.6f, \"accuracy\": %d}"
                "}, "
                "\"services\": {\"police\": %s, \"fire\": %s}"
            "}",
            conf->name, conf->phone, conf->AccountID,
            conf->street, conf->city, conf->state, conf->zip,
            conf->Latitude, conf->Longitude, conf->Accuracy,
            police, fire
        );

        curl_easy_setopt(curl, CURLOPT_URL, conf->monitoringURL);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response_buffer);

        printf("[NOONLIGHT] Posting Alarm for: %s\n", z->name);
        CURLcode res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            cJSON *root = cJSON_Parse(response_buffer);
            if (root) {
                cJSON *id = cJSON_GetObjectItem(root, "id");
                if (id && id->valuestring) {
                    strncpy(active_alarm_id, id->valuestring, STR_SMALL - 1);
                    success = true;
                    printf("[NOONLIGHT] Alarm Success. ID: %s\n", active_alarm_id);
                }
                cJSON_Delete(root);
            }
        } else {
            printf("[NOONLIGHT] CURL Error: %s\n", curl_easy_strerror(res));
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return success;
}

// --- 2. LOG EVENT ---
void noonlight_log_event(config_t *conf, zone_t *z) {
    if (strlen(active_alarm_id) == 0) return;

    CURL *curl;
    struct curl_slist *headers = NULL;
    curl = setup_noonlight_curl(conf, &headers);

    if (curl) {
        char url[256], data[512];
        snprintf(url, sizeof(url), "%s/%s/events", conf->monitoringURL, active_alarm_id);
        
        snprintf(data, sizeof(data), 
            "[{\"event_type\": \"alarm.device.activated_alarm\", "
            "\"meta\": {\"attribute\": \"%s\", \"value\": \"tripped\", \"device_name\": \"%s\"}}]", 
            z->type, z->name);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

// --- 3. SEND INSTRUCTIONS ---
void noonlight_send_instructions(config_t *conf) {
    if (strlen(active_alarm_id) == 0 || strlen(conf->instructions) == 0) return;

    CURL *curl;
    struct curl_slist *headers = NULL;
    curl = setup_noonlight_curl(conf, &headers);

    if (curl) {
        char url[256], data[512];
        snprintf(url, sizeof(url), "%s/%s/instructions", conf->monitoringURL, active_alarm_id);
        snprintf(data, sizeof(data), "{\"instruction\": \"%s\"}", conf->instructions);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

// --- 4. SYNC PEOPLE (BACKUP CONTACTS) ---
void noonlight_sync_people(config_t *conf, user_t *users_list, int count) {
    if (strlen(active_alarm_id) == 0) return;

    for (int i = 0; i < count; i++) {
        CURL *curl;
        struct curl_slist *headers = NULL;
        curl = setup_noonlight_curl(conf, &headers);

        if (curl) {
            char url[256], data[512];
            snprintf(url, sizeof(url), "%s/%s/people", conf->monitoringURL, active_alarm_id);
            snprintf(data, sizeof(data), 
                "{\"name\": \"%s\", \"phone\": \"%s\", \"pin\": \"%s\"}", 
                users_list[i].name, users_list[i].phone, users_list[i].pin);

            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            curl_easy_perform(curl);
            
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
    }
}

// --- 5. CANCEL ALARM ---
bool noonlight_cancel_alarm(config_t *conf, const char *pin) {
    if (strlen(active_alarm_id) == 0) return false;

    CURL *curl;
    struct curl_slist *headers = NULL;
    curl = setup_noonlight_curl(conf, &headers);

    if (curl) {
        char url[256], data[128];
        snprintf(url, sizeof(url), "%s/%s/status", conf->monitoringURL, active_alarm_id);
        snprintf(data, sizeof(data), "{\"status\": \"canceled\", \"pin\": \"%s\"}", pin);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            printf("[NOONLIGHT] Alarm Canceled successfully.\n");
            memset(active_alarm_id, 0, sizeof(active_alarm_id));
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return (res == CURLE_OK);
    }
    return false;
}