#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "dispatcher.h"
#include "storage_mgr.h"

#define LOAD_STR(dest, json, key) do { \
    cJSON *temp = cJSON_GetObjectItem(json, key); \
    if (temp && cJSON_IsString(temp)) { \
        strncpy(dest, temp->valuestring, sizeof(dest) - 1); \
        dest[sizeof(dest) - 1] = '\0'; \
    } \
} while(0)

#define LOAD_INT(dest, json, key) do { \
    cJSON *temp = cJSON_GetObjectItem(json, key); \
    if (temp && cJSON_IsNumber(temp)) dest = temp->valueint; \
} while(0)

#define LOAD_DBL(dest, json, key) do { \
    cJSON *temp = cJSON_GetObjectItem(json, key); \
    if (temp && cJSON_IsNumber(temp)) dest = temp->valuedouble; \
} while(0)

#define LOAD_BOOL(dest, json, key) do { \
    cJSON *temp = cJSON_GetObjectItem(json, key); \
    if (temp) dest = cJSON_IsTrue(temp); \
} while(0)

static char* read_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len + 1);
    fread(data, 1, len, f);
    fclose(f);
    data[len] = '\0';
    return data;
}

void storage_load_owner(owner_t *obj) {
    char *data = read_file("data/owner.json");
    if (!data) return;
    cJSON *json = cJSON_Parse(data);
    if (!json) { free(data); return; }

    LOAD_STR(obj->account_id, json, "AccountID");
    LOAD_STR(obj->name, json, "Name");
    LOAD_STR(obj->address1, json, "Address1");
    LOAD_STR(obj->address2, json, "Address2");
    LOAD_STR(obj->city, json, "City");
    LOAD_STR(obj->state, json, "State");
    LOAD_STR(obj->zip, json, "ZipCode");
    LOAD_DBL(obj->latitude, json, "Latitude");
    LOAD_DBL(obj->longitude, json, "Longitude");
    LOAD_STR(obj->monitor_service_key, json, "MonitorServiceKey");
    LOAD_STR(obj->monitoring_url, json, "MonitoringURL");
    LOAD_BOOL(obj->monitor_police, json, "MonitorPolice");
    LOAD_STR(obj->smtp_server, json, "SMTPServer");
    LOAD_INT(obj->smtp_port, json, "SMTPPort");
    LOAD_STR(obj->smtp_user, json, "SMTPUser");
    LOAD_STR(obj->smtp_pass, json, "SMTPPass");
    LOAD_INT(obj->entry_delay, json, "EntryDelay");

    cJSON_Delete(json);
    free(data);
}

void storage_load_users(user_t *list, int max, int *count) {
    char *data = read_file("data/users.json");
    if (!data) return;
    cJSON *root = cJSON_Parse(data);
    int i = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, root) {
        if (i >= max) break;
        LOAD_STR(list[i].name, item, "name");
        LOAD_STR(list[i].pin, item, "pin");
        LOAD_BOOL(list[i].is_admin, item, "is_admin");
        i++;
    }
    *count = i;
    cJSON_Delete(root);
    free(data);
}