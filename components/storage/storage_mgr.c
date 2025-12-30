#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "storage_mgr.h"

// 1. Define the actual memory for the shared variables
owner_t owner;
zone_t zones[MAX_ZONES];
user_t users[MAX_USERS];
relay_t relays[MAX_RELAYS];

int z_count = 0;
int u_count = 0;
int r_count = 0;

// Helper function to read file into a string
static char* read_file_to_string(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("[STORAGE] Error: Could not open %s\n", filename);
        return NULL;
    }
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

// --- LOAD OWNER ---
void storage_load_owner(owner_t *obj) {
    char *data = read_file_to_string("data/owner.json");
    if (!data) return;
    printf("[STORAGE] Raw Owner Data: %s\n", data);

    cJSON *root = cJSON_Parse(data);
    if (root) {
        cJSON *email = cJSON_GetObjectItem(root, "Email");
        cJSON *pin   = cJSON_GetObjectItem(root, "PIN");
        cJSON *svr   = cJSON_GetObjectItem(root, "SMTP_Server");
        cJSON *user  = cJSON_GetObjectItem(root, "SMTP_User");
        cJSON *pass  = cJSON_GetObjectItem(root, "SMTP_Pass");
        cJSON *port  = cJSON_GetObjectItem(root, "SMTP_Port");

        if (email) strncpy(obj->email, email->valuestring, STR_MEDIUM - 1);
        if (pin)   obj->pin = pin->valueint;
        if (svr)   strncpy(obj->smtp_server, svr->valuestring, STR_MEDIUM - 1);
        if (user)  strncpy(obj->smtp_user, user->valuestring, STR_MEDIUM - 1);
        if (pass)  strncpy(obj->smtp_pass, pass->valuestring, STR_MEDIUM - 1);
        if (port)  obj->smtp_port = port->valueint;

        cJSON_Delete(root);
    }
    free(data);
}

// --- LOAD ZONES ---
void storage_load_zones(zone_t *list, int *count) {
    char *data = read_file_to_string("data/zones.json");
    if (!data) { *count = 0; return; }
    printf("[STORAGE] Raw Zones Data: %s\n", data);

    cJSON *root = cJSON_Parse(data);
    if (!root) { free(data); return; }

    int i = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        if (i >= MAX_ZONES) break;
        cJSON *id   = cJSON_GetObjectItem(item, "ID");
        cJSON *name = cJSON_GetObjectItem(item, "Name");
        cJSON *type = cJSON_GetObjectItem(item, "Type");

        if (id)   list[i].id = id->valueint;
        if (name) strncpy(list[i].name, name->valuestring, STR_MEDIUM - 1);
        if (type) strncpy(list[i].type, type->valuestring, STR_SMALL - 1);
        i++;
    }
    *count = i;
    cJSON_Delete(root);
    free(data);
}

// --- LOAD USERS ---
void storage_load_users(user_t *list, int *count) {
    char *data = read_file_to_string("data/users.json");
    if (!data) { *count = 0; return; }
    printf("[STORAGE] Raw Users Data: %s\n", data);

    cJSON *root = cJSON_Parse(data);
    if (!root) { free(data); return; }

    int i = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        if (i >= MAX_USERS) break;
        cJSON *name  = cJSON_GetObjectItem(item, "Name");
        cJSON *pin   = cJSON_GetObjectItem(item, "PIN");
        cJSON *email = cJSON_GetObjectItem(item, "Email");
        cJSON *phone = cJSON_GetObjectItem(item, "Phone");

        if (name)  strncpy(list[i].name,  name->valuestring,  STR_MEDIUM - 1);
        if (pin)   strncpy(list[i].pin,   pin->valuestring,   STR_SMALL - 1);
        if (email) strncpy(list[i].email, email->valuestring, STR_MEDIUM - 1);
        if (phone) strncpy(list[i].phone, phone->valuestring, STR_SMALL - 1);
        i++;
    }
    *count = i;
    cJSON_Delete(root);
    free(data);
}

// --- LOAD RELAYS ---
void storage_load_relays(relay_t *list, int *count) {
    char *data = read_file_to_string("data/relays.json");
    if (!data) { *count = 0; return; }
    printf("[STORAGE] Raw Relays Data: %s\n", data);

    cJSON *root = cJSON_Parse(data);
    if (!root) { free(data); return; }

    int i = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root) {
        if (i >= MAX_RELAYS) break;
        cJSON *id   = cJSON_GetObjectItem(item, "ID");
        cJSON *type = cJSON_GetObjectItem(item, "Type");

        if (id)   list[i].id = id->valueint;
        if (type) strncpy(list[i].type, type->valuestring, STR_SMALL - 1);
        i++;
    }
    *count = i;
    cJSON_Delete(root);
    free(data);
}

// --- LOAD ALL ---
void storage_load_all(owner_t *o, zone_t *z, int *zc, user_t *u, int *uc, relay_t *r, int *rc) {
    storage_load_owner(o);
    storage_load_zones(z, zc);
    storage_load_users(u, uc);
    storage_load_relays(r, rc);
}