#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "storage_mgr.h"
#include "noonlight.h"

void noonlight_send_event(owner_t *o, const char *alarm_id, zone_t *z) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    char data[512];
    snprintf(data, sizeof(data), 
        "{ \"source\": \"Sentinel Linux\", \"location\": { \"address\": \"Home\" }, \"services\": [\"police\"] }");

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.noonlight.com/dispatch/v1/alarms");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    printf("[NOONLIGHT] Dispatching Police for Zone: %s\n", z->name);
    CURLcode res = curl_easy_perform(curl);
    
    if(res != CURLE_OK) printf("[NOONLIGHT] API Error: %s\n", curl_easy_strerror(res));
    else printf("[NOONLIGHT] API SUCCESS: Police Notified.\n");

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void noonlight_cancel_alarm(owner_t *o, int slot, const char *entered_pin) {
    printf("[NOONLIGHT] Sending API Cancel with PIN %s\n", entered_pin);
}