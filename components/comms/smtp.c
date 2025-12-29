#include "storage_mgr.h"
#include "smtp.h"
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

void smtp_send_email(owner_t *owner, const char *subject, const char *body) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    struct curl_slist *recipients = NULL;
    char url[256];
    snprintf(url, sizeof(url), "smtps://%s:%d", owner->smtp_server, owner->smtp_port);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERNAME, owner->smtp_user);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, owner->smtp_pass);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, owner->smtp_user);

    recipients = curl_slist_append(recipients, owner->email);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    char payload[2048];
    snprintf(payload, sizeof(payload),
             "To: %s\r\nFrom: %s\r\nSubject: %s\r\n\r\n%s",
             owner->email, owner->smtp_user, subject, body);

    FILE *temp = fmemopen((void*)payload, strlen(payload), "r");
    curl_easy_setopt(curl, CURLOPT_READDATA, temp);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    printf("[SMTP] Sending via %s...\n", url);
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) printf("[SMTP] Error: %s\n", curl_easy_strerror(res));
    else printf("[SMTP] Success: Email sent to %s\n", owner->email);

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
    fclose(temp);
}