#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cJSON.h"
#include "dispatcher.h"

char* read_file_to_buffer(const char* filename) {
    // Debug: Print current working directory and target
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("[STORAGE] CWD: %s\n", cwd);
    }
    printf("[STORAGE] Attempting to open: %s\n", filename);

    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("[STORAGE] ERROR: Could not open file %s\n", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len + 1);
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);
    return data;
}

void storage_load_owner(config_t *obj) {
    char *data = read_file_to_buffer("../data/config.json");
    if (!data) return;
    cJSON *root = cJSON_Parse(data);
    if (root) {
        cJSON *item;
        if ((item = cJSON_GetObjectItem(root, "Notify"))) strncpy(obj->notify, item->valuestring, STR_SMALL - 1);
        if ((item = cJSON_GetObjectItem(root, "name"))) strncpy(obj->name, item->valuestring, STR_MEDIUM - 1);
        if ((item = cJSON_GetObjectItem(root, "Email"))) strncpy(obj->email, item->valuestring, STR_MEDIUM - 1);
        if ((item = cJSON_GetObjectItem(root, "SMTPServer"))) strncpy(obj->smtp_server, item->valuestring, STR_MEDIUM - 1);
        if ((item = cJSON_GetObjectItem(root, "SMTPPort"))) obj->smtp_port = item->valueint;
        if ((item = cJSON_GetObjectItem(root, "SMTPUser"))) strncpy(obj->smtp_user, item->valuestring, STR_MEDIUM - 1);
        if ((item = cJSON_GetObjectItem(root, "SMTPPass"))) strncpy(obj->smtp_pass, item->valuestring, STR_MEDIUM - 1);
        cJSON_Delete(root);
    }
    free(data);
}

void storage_load_users(user_t *u, int *count) {
    char *data = read_file_to_buffer("../data/users.json");
    if (!data) { *count = 0; return; }
    
    printf("[STORAGE] Raw Data Loaded: %s\n", data); // DEBUG PRINT

    cJSON *root = cJSON_Parse(data);
    if (!root) {
        printf("[STORAGE] ERROR: cJSON Parse failed!\n");
        free(data);
        *count = 0;
        return;
    }

    int found = 0;
    if (cJSON_IsArray(root)) {
        cJSON *element = NULL;
        cJSON_ArrayForEach(element, root) {
            if (found < MAX_USER) {
                // Use the exact keys from your snippet: "name" and "Phone"
                cJSON *name = cJSON_GetObjectItem(element, "name");
                cJSON *phone = cJSON_GetObjectItem(element, "Phone");
                
                if (name && phone) {
                    strncpy(u[found].name, name->valuestring, STR_MEDIUM - 1);
                    strncpy(u[found].phone, phone->valuestring, STR_SMALL - 1);
                    printf("[STORAGE] Successfully Parsed User: %s\n", u[found].name);
                    found++;
                } else {
                    printf("[STORAGE] Warning: Element missing name or Phone key\n");
                }
            }
        }
    } else {
        printf("[STORAGE] ERROR: root is not an array\n");
    }

    *count = found;
    cJSON_Delete(root);
    free(data);
}

void storage_load_zones(zone_t *z, int max) {
    z[0].id = 1;
    strcpy(z[0].name, "Front Door");
    strcpy(z[0].type, "Entry");
    strcpy(z[0].location, "Foyer");
}
