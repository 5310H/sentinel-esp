#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "dispatcher.h"

/* --- Safety Macros --- */
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
    if (temp && cJSON_IsBool(temp)) dest = cJSON_IsTrue(temp); \
} while(0)

/* --- Core Loading Logic --- */
char* read_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len + 1);
    if (data) {
        fread(data, 1, len, f);
        data[len] = '\0';
    }
    fclose(f);
    return data;
}

void storage_load_owner(owner_t *obj) {
    char *data = read_file("../data/owner.json");
    if (!data) {
        printf("[STORAGE] Error: Could not load owner.json\n");
        return;
    }

    cJSON *json = cJSON_Parse(data);
    if (json) {
        LOAD_STR(obj->name, json, "Name");
        LOAD_STR(obj->email, json, "Email");
        LOAD_STR(obj->address1, json, "Address1");
        LOAD_STR(obj->city, json, "City");
        
        LOAD_DBL(obj->latitude, json, "Latitude");
        LOAD_DBL(obj->longitude, json, "Longitude");

        LOAD_STR(obj->monitoring_url, json, "MonitoringURL");
        LOAD_STR(obj->monitor_service_key, json, "MonitorServiceKey");

        LOAD_STR(obj->smtp_server, json, "SMTPServer");
        LOAD_INT(obj->smtp_port, json, "SMTPPort");
        LOAD_STR(obj->smtp_user, json, "SMTPUser");
        LOAD_STR(obj->smtp_pass, json, "SMTPPass");

        cJSON_Delete(json);
    }
    free(data);
}

void storage_load_zones(zone_t *list, int *count) {
    char *data = read_file("../data/zones.json");
    if (!data) return;

    cJSON *json = cJSON_Parse(data);
    if (json && cJSON_IsArray(json)) {
        *count = cJSON_GetArraySize(json);
        for (int i = 0; i < *count && i < 16; i++) {
            cJSON *item = cJSON_GetArrayItem(json, i);
            LOAD_INT(list[i].id, item, "id");
            LOAD_STR(list[i].name, item, "name");
            LOAD_STR(list[i].location, item, "location");
        }
        cJSON_Delete(json);
    }
    free(data);
}