#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include "notifier.h"

// --- CONFIGURATION ---
#define NOONLIGHT_TOKEN     "YOUR_NOONLIGHT_TOKEN"
#define NOONLIGHT_URL       "https://api-sandbox.noonlight.com/dispatch/v1/alarms"
#define SMTP_SERVER         "smtp://smtp.gmail.com:587"
#define SMTP_USER           "your_email@gmail.com"
#define SMTP_PASS           "your_app_password"
#define EMAIL_TO            "receiver@example.com"
#define EMAIL_FROM          "your_email@gmail.com"
#define NOTIFY_COOLDOWN_SEC 300 

// --- INTERNAL STATE ---
static char _current_alarm_id[128] = "";
static char _account_id[64] = "";
static time_t _last_alarm_time = 0;

struct EmailData {
    char content[2048];
    size_t pos;
};

// --- CALLBACKS ---
static size_t noonlight_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    char *id_ptr = strstr((char*)contents, "\"id\":\"");
    if (id_ptr) sscanf(id_ptr, "\"id\":\"%[^\"]\"", (char*)userp);
    return size * nmemb;
}

static size_t smtp_cb(void *ptr, size_t size, size_t nmemb, void *userp) {
    struct EmailData *data = (struct EmailData *)userp;
    size_t len = strlen(data->content + data->pos);
    if (len > 0) {
        size_t to_copy = (len < size * nmemb) ? len : size * nmemb;
        memcpy(ptr, data->content + data->pos, to_copy);
        data->pos += to_copy;
        return to_copy;
    }
    return 0;
}

// --- IMPLEMENTATION ---
void notifier_init(const char* url, const char* account_id) {
    strncpy(_account_id, account_id, 63);
    curl_global_init(CURL_GLOBAL_ALL);
}

void notifier_send_email(const char* subject, const char* body) {
    CURL *curl = curl_easy_init();
    if(!curl) return;

    struct EmailData email;
    time_t now = time(NULL);
    snprintf(email.content, sizeof(email.content),
             "From: " EMAIL_FROM "\r\nTo: " EMAIL_TO "\r\nSubject: %s\r\n\r\n"
             "Timestamp: %sAccount: %s\r\n\r\n%s", 
             subject, ctime(&now), _account_id, body);
    email.pos = 0;

    curl_easy_setopt(curl, CURLOPT_URL, SMTP_SERVER);
    curl_easy_setopt(curl, CURLOPT_USERNAME, SMTP_USER);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, SMTP_PASS);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, EMAIL_FROM);
    struct curl_slist *recipients = curl_slist_append(NULL, EMAIL_TO);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, smtp_cb);
    curl_easy_setopt(curl, CURLOPT_READDATA, &email);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    if(curl_easy_perform(curl) != CURLE_OK) printf("[NOTIFIER] Email Failed\n");
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
}

static void dispatch_noonlight(const char* message) {
    CURL *curl = curl_easy_init();
    if(!curl) return;
    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
    char auth[256]; snprintf(auth, sizeof(auth), "Authorization: Bearer %s", NOONLIGHT_TOKEN);
    headers = curl_slist_append(headers, auth);

    char json[1024];
    snprintf(json, sizeof(json), "{\"name\":\"Kevin\",\"phone\":\"3145550199\","
             "\"location\":{\"address\":{\"line1\":\"123 Test St\",\"city\":\"St. Louis\",\"state\":\"MO\",\"zip\":\"63104\"}},"
             "\"note\":\"%s\"}", message);

    curl_easy_setopt(curl, CURLOPT_URL, NOONLIGHT_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noonlight_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)_current_alarm_id);

    if(curl_easy_perform(curl) == CURLE_OK) printf("[NOTIFIER] Noonlight ID: %s\n", _current_alarm_id);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
}

void notifier_send(NotifyType_t type, const char* message) {
    time_t now = time(NULL);
    if (type == NOTIFY_ALARM) {
        if (_last_alarm_time != 0 && difftime(now, _last_alarm_time) < NOTIFY_COOLDOWN_SEC) return;
        _last_alarm_time = now;
        dispatch_noonlight(message);
        notifier_send_email("SECURITY ALARM", message);
    }
    printf("[%s] %s\n", (type == NOTIFY_ALARM) ? "ALARM" : "INFO", message);
}

bool notifier_cancel_alarm(void) {
    _last_alarm_time = 0; 
    if (strlen(_current_alarm_id) == 0) return false;
    CURL *curl = curl_easy_init();
    if(!curl) return false;
    char url[256]; snprintf(url, sizeof(url), "%s/%s/status", NOONLIGHT_URL, _current_alarm_id);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"status\":\"canceled\"}");
    bool res = (curl_easy_perform(curl) == CURLE_OK);
    if(res) _current_alarm_id[0] = '\0';
    curl_easy_cleanup(curl);
    return res;
}