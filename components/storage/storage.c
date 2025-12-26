#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

static char* read_file_to_string(const char* path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(size + 1);
    if (!data) { fclose(f); return NULL; }
    size_t n = fread(data, 1, size, f);
    fclose(f);
    data[n] = '\0';
    return data;
}

static void safe_strcpy(char* dest, cJSON* obj, const char* key, size_t max_len) {
    cJSON* item = cJSON_GetObjectItem(obj, key);
    if (item && item->valuestring) {
        strncpy(dest, item->valuestring, max_len - 1);
        dest[max_len - 1] = '\0';
    }
}

bool storage_load_all(Owner_t *o, User_t *u, int *uc, Zone_t *z, int *zc, Relay_t *r, int *rc, Rule_t *ru, int *ruc) {
    char *data = read_file_to_string("data/setup.json");
    if (!data) return 1;
    cJSON *root = cJSON_Parse(data);
    if (!root) { free(data); return 1; }

    cJSON *own = cJSON_GetObjectItem(root, "owner");
    if(own) {
        safe_strcpy(o->AccountID, own, "AccountID", 64);
        o->PIN = cJSON_GetObjectItem(own, "PIN") ? cJSON_GetObjectItem(own, "PIN")->valueint : 0;
        safe_strcpy(o->Name, own, "Name", 64);
        safe_strcpy(o->Email, own, "Email", 64);
        o->Latitude = cJSON_GetObjectItem(own, "Latitude") ? cJSON_GetObjectItem(own, "Latitude")->valuedouble : 0.0;
        o->Longitude = cJSON_GetObjectItem(own, "Longitude") ? cJSON_GetObjectItem(own, "Longitude")->valuedouble : 0.0;
        o->EntryDelay = cJSON_GetObjectItem(own, "EntryDelay") ? cJSON_GetObjectItem(own, "EntryDelay")->valueint : 30;
        o->ExitDelay = cJSON_GetObjectItem(own, "ExitDelay") ? cJSON_GetObjectItem(own, "ExitDelay")->valueint : 60;
    }

    cJSON *item;
    cJSON *u_arr = cJSON_GetObjectItem(root, "users");
    *uc = 0;
    if (cJSON_IsArray(u_arr)) {
        cJSON_ArrayForEach(item, u_arr) {
            if (*uc >= MAX_USERS) break;
            u[*uc].ID = cJSON_GetObjectItem(item, "ID") ? cJSON_GetObjectItem(item, "ID")->valueint : 0;
            safe_strcpy(u[*uc].Name, item, "Name", 32);
            safe_strcpy(u[*uc].PIN, item, "PIN", 8);
            u[*uc].IsAdmin = cJSON_IsTrue(cJSON_GetObjectItem(item, "IsAdmin"));
            (*uc)++;
        }
    }

    cJSON *z_arr = cJSON_GetObjectItem(root, "zones");
    *zc = 0;
    if (cJSON_IsArray(z_arr)) {
        cJSON_ArrayForEach(item, z_arr) {
            if (*zc >= MAX_ZONES) break;
            z[*zc].ID = cJSON_GetObjectItem(item, "ID") ? cJSON_GetObjectItem(item, "ID")->valueint : 0;
            safe_strcpy(z[*zc].Name, item, "Name", 32);
            z[*zc].PinNumber = cJSON_GetObjectItem(item, "PinNumber") ? cJSON_GetObjectItem(item, "PinNumber")->valueint : 0;
            z[*zc].IsPerimeter = cJSON_IsTrue(cJSON_GetObjectItem(item, "IsPerimeter"));
            (*zc)++;
        }
    }

    cJSON *r_arr = cJSON_GetObjectItem(root, "relays");
    *rc = 0;
    if (cJSON_IsArray(r_arr)) {
        cJSON_ArrayForEach(item, r_arr) {
            if (*rc >= MAX_RELAYS) break;
            r[*rc].ID = cJSON_GetObjectItem(item, "ID") ? cJSON_GetObjectItem(item, "ID")->valueint : 0;
            safe_strcpy(r[*rc].Name, item, "Name", 32);
            r[*rc].PinNumber = cJSON_GetObjectItem(item, "PinNumber") ? cJSON_GetObjectItem(item, "PinNumber")->valueint : 0;
            (*rc)++;
        }
    }

    cJSON *ru_arr = cJSON_GetObjectItem(root, "rules");
    *ruc = 0;
    if (cJSON_IsArray(ru_arr)) {
        cJSON_ArrayForEach(item, ru_arr) {
            if (*ruc >= MAX_RULES) break;
            ru[*ruc].ID = cJSON_GetObjectItem(item, "ID") ? cJSON_GetObjectItem(item, "ID")->valueint : 0;
            safe_strcpy(ru[*ruc].Name, item, "Name", 32);
            ru[*ruc].Enabled = cJSON_IsTrue(cJSON_GetObjectItem(item, "Enabled"));
            ru[*ruc].TriggerZoneID = cJSON_GetObjectItem(item, "TriggerZoneID") ? cJSON_GetObjectItem(item, "TriggerZoneID")->valueint : 0;
            ru[*ruc].ActionRelayID = cJSON_GetObjectItem(item, "ActionRelayID") ? cJSON_GetObjectItem(item, "ActionRelayID")->valueint : 0;
            (*ruc)++;
        }
    }

    cJSON_Delete(root);
    free(data);
    return 0;
}

bool storage_save_all(Owner_t *o, User_t *u, int uc, Zone_t *z, int zc, Relay_t *r, int rc, Rule_t *ru, int ruc) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON *own = cJSON_CreateObject();
    cJSON_AddStringToObject(own, "AccountID", o->AccountID);
    cJSON_AddNumberToObject(own, "PIN", o->PIN);
    cJSON_AddStringToObject(own, "Name", o->Name);
    cJSON_AddNumberToObject(own, "Latitude", o->Latitude);
    cJSON_AddNumberToObject(own, "Longitude", o->Longitude);
    cJSON_AddNumberToObject(own, "EntryDelay", o->EntryDelay);
    cJSON_AddNumberToObject(own, "ExitDelay", o->ExitDelay);
    cJSON_AddItemToObject(root, "owner", own);

    cJSON *u_arr = cJSON_CreateArray();
    for (int i = 0; i < uc; i++) {
        cJSON *it = cJSON_CreateObject();
        cJSON_AddNumberToObject(it, "ID", u[i].ID);
        cJSON_AddStringToObject(it, "Name", u[i].Name);
        cJSON_AddStringToObject(it, "PIN", u[i].PIN);
        cJSON_AddBoolToObject(it, "IsAdmin", u[i].IsAdmin);
        cJSON_AddItemToArray(u_arr, it);
    }
    cJSON_AddItemToObject(root, "users", u_arr);

    cJSON *z_arr = cJSON_CreateArray();
    for (int i = 0; i < zc; i++) {
        cJSON *it = cJSON_CreateObject();
        cJSON_AddNumberToObject(it, "ID", z[i].ID);
        cJSON_AddStringToObject(it, "Name", z[i].Name);
        cJSON_AddNumberToObject(it, "PinNumber", z[i].PinNumber);
        cJSON_AddBoolToObject(it, "IsPerimeter", z[i].IsPerimeter);
        cJSON_AddItemToArray(z_arr, it);
    }
    cJSON_AddItemToObject(root, "zones", z_arr);

    cJSON *r_arr = cJSON_CreateArray();
    for (int i = 0; i < rc; i++) {
        cJSON *it = cJSON_CreateObject();
        cJSON_AddNumberToObject(it, "ID", r[i].ID);
        cJSON_AddStringToObject(it, "Name", r[i].Name);
        cJSON_AddNumberToObject(it, "PinNumber", r[i].PinNumber);
        cJSON_AddItemToArray(r_arr, it);
    }
    cJSON_AddItemToObject(root, "relays", r_arr);

    cJSON *ru_arr = cJSON_CreateArray();
    for (int i = 0; i < ruc; i++) {
        cJSON *it = cJSON_CreateObject();
        cJSON_AddNumberToObject(it, "ID", ru[i].ID);
        cJSON_AddStringToObject(it, "Name", ru[i].Name);
        cJSON_AddBoolToObject(it, "Enabled", ru[i].Enabled);
        cJSON_AddNumberToObject(it, "TriggerZoneID", ru[i].TriggerZoneID);
        cJSON_AddNumberToObject(it, "ActionRelayID", ru[i].ActionRelayID);
        cJSON_AddItemToArray(ru_arr, it);
    }
    cJSON_AddItemToObject(root, "rules", ru_arr);

    char *rendered = cJSON_Print(root);
    FILE *f = fopen("data/setup.json.bak", "w");
    if (f) {
        fprintf(f, "%s", rendered);
        fclose(f);
        printf("[STORAGE] Full backup written to data/setup.json.bak\n");
    }
    free(rendered);
    cJSON_Delete(root);
    return true;
}

bool storage_save_owner(Owner_t* owner) { return true; }
bool storage_save_users(User_t* users, int count) { return true; }
