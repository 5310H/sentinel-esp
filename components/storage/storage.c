#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "storage.h"

// Internal helper to read file
static char* read_file_to_string(const char* path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len + 1);
    if (!data) { fclose(f); return NULL; }
    fread(data, 1, len, f);
    fclose(f);
    data[len] = '\0';
    return data;
}

bool storage_load_all(Owner_t *o, User_t *u, int *uc, Zone_t *z, int *zc, Relay_t *r, int *rc, Rule_t *ru, int *ruc) {
    char *data = read_file_to_string("data/setup.json");
    if (!data) return false;

    cJSON *root = cJSON_Parse(data);
    if (!root) { free(data); return false; }

    // --- Owner ---
    cJSON *own = cJSON_GetObjectItem(root, "owner");
    if(own) {
        strncpy(o->AccountID, cJSON_GetObjectItem(own, "AccountID")->valuestring, 63);
        o->ExitDelay = cJSON_GetObjectItem(own, "ExitDelay")->valueint;
        o->EntryDelay = cJSON_GetObjectItem(own, "EntryDelay")->valueint;
        o->Latitude = cJSON_GetObjectItem(own, "Latitude")->valuedouble;
        o->Longitude = cJSON_GetObjectItem(own, "Longitude")->valuedouble;
        strncpy(o->MonitoringURL, cJSON_GetObjectItem(own, "MonitoringURL")->valuestring, 127);
    }

    cJSON *item;

    // --- Users ---
    cJSON *u_arr = cJSON_GetObjectItem(root, "users");
    *uc = 0;
    cJSON_ArrayForEach(item, u_arr) {
        if (*uc >= MAX_USERS) break;
        u[*uc].ID = cJSON_GetObjectItem(item, "ID")->valueint;
        strncpy(u[*uc].Name, cJSON_GetObjectItem(item, "Name")->valuestring, 31);
        strncpy(u[*uc].PIN, cJSON_GetObjectItem(item, "PIN")->valuestring, 7);
        u[*uc].IsAdmin = cJSON_IsTrue(cJSON_GetObjectItem(item, "IsAdmin"));
        (*uc)++;
    }

    // --- Zones ---
    cJSON *z_arr = cJSON_GetObjectItem(root, "zones");
    *zc = 0;
    cJSON_ArrayForEach(item, z_arr) {
        if (*zc >= MAX_ZONES) break;
        z[*zc].ID = cJSON_GetObjectItem(item, "ID")->valueint;
        z[*zc].PinNumber = cJSON_GetObjectItem(item, "PinNumber")->valueint;
        z[*zc].IsPerimeter = cJSON_IsTrue(cJSON_GetObjectItem(item, "IsPerimeter"));
        strncpy(z[*zc].Name, cJSON_GetObjectItem(item, "Name")->valuestring, 31);
        (*zc)++;
    }

    // --- Relays ---
    cJSON *r_arr = cJSON_GetObjectItem(root, "relays");
    *rc = 0;
    cJSON_ArrayForEach(item, r_arr) {
        if (*rc >= MAX_RELAYS) break;
        r[*rc].ID = cJSON_GetObjectItem(item, "ID")->valueint;
        r[*rc].PinNumber = cJSON_GetObjectItem(item, "PinNumber")->valueint;
        strncpy(r[*rc].Name, cJSON_GetObjectItem(item, "Name")->valuestring, 31);
        (*rc)++;
    }

    // --- Rules (The Fixed Section) ---
    cJSON *ru_arr = cJSON_GetObjectItem(root, "rules");
    *ruc = 0;
    cJSON_ArrayForEach(item, ru_arr) {
        if (*ruc >= MAX_RULES) break;
        ru[*ruc].Enabled = cJSON_IsTrue(cJSON_GetObjectItem(item, "Enabled"));
        ru[*ruc].TriggerZoneID = cJSON_GetObjectItem(item, "TriggerZoneID")->valueint;
        ru[*ruc].ActionRelayID = cJSON_GetObjectItem(item, "ActionRelayID")->valueint;
        strncpy(ru[*ruc].Name, cJSON_GetObjectItem(item, "Name")->valuestring, 31);
        (*ruc)++;
    }

    cJSON_Delete(root);
    free(data);
    return true;
}

// Add these empty stubs so the linker doesn't complain about storage.h declarations
bool storage_save_owner(Owner_t* owner) { return true; }
bool storage_save_users(User_t* users, int count) { return true; }
