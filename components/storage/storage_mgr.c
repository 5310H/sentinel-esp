#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "storage_mgr.h"

// Global memory allocation
config_t config;
zone_t zones[MAX_ZONES];
user_t users[MAX_USERS];
relay_t relays[MAX_RELAYS];

int z_count = 0;
int u_count = 0;
int r_count = 0;

/**
 * HELPER: Safe String Copy
 * Prevents Segfaults by checking if cJSON item is a valid string before copying.
 */
static void safe_strncpy(char *dest, cJSON *item, size_t max_len) {
    if (item != NULL && cJSON_IsString(item) && item->valuestring != NULL) {
        strncpy(dest, item->valuestring, max_len - 1);
        dest[max_len - 1] = '\0';
    }
}

/**
 * HELPER: Safe Number Get
 * Prevents logic errors by checking if item is a number.
 */
static int safe_get_int(cJSON *item, int default_val) {
    return (item != NULL && cJSON_IsNumber(item)) ? item->valueint : default_val;
}

// Helper to read file from local disk (Linux) or SPIFFS (ESP32)
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

// --- LOAD MAIN CONFIG ---
void storage_load_config() {
    char *data = read_file_to_string("data/config.json");
    if (!data) return;
    printf("\n[DEBUG] Raw File Content: %s\n", data);
    cJSON *root = cJSON_Parse(data);
    if (root) {
        // Identity & Contact
        safe_strncpy(config.name,  cJSON_GetObjectItem(root, "name"),  STR_MEDIUM);
        safe_strncpy(config.email, cJSON_GetObjectItem(root, "email"), STR_MEDIUM);
        safe_strncpy(config.pin,   cJSON_GetObjectItem(root, "pin"),   STR_SMALL);
        safe_strncpy(config.phone, cJSON_GetObjectItem(root, "phone"), STR_SMALL);
        
        // Location
        config.Latitude  = (cJSON_GetObjectItem(root, "Latitude"))  ? cJSON_GetObjectItem(root, "Latitude")->valuedouble : 0.0;
        config.Longitude = (cJSON_GetObjectItem(root, "Longitude")) ? cJSON_GetObjectItem(root, "Longitude")->valuedouble : 0.0;
        safe_strncpy(config.street, cJSON_GetObjectItem(root, "street"), STR_MEDIUM);
        safe_strncpy(config.city,   cJSON_GetObjectItem(root, "city"),   STR_SMALL);

        // Logic & Timing (The cause of your previous crash)
        config.EntryDelay   = safe_get_int(cJSON_GetObjectItem(root, "EntryDelay"), 30);
        config.ExitDelay    = safe_get_int(cJSON_GetObjectItem(root, "ExitDelay"), 60);
        config.SirenTimeout = safe_get_int(cJSON_GetObjectItem(root, "SirenTimeout"), 300);

        // Telegram & SMTP
        safe_strncpy(config.TelegramToken, cJSON_GetObjectItem(root, "TelegramToken"), STR_MEDIUM);
        cJSON *t_en = cJSON_GetObjectItem(root, "TelegramEnabled");
        config.TelegramEnabled = (t_en && cJSON_IsTrue(t_en));

        cJSON_Delete(root);
    }
    free(data);
}

// --- LOAD ZONES ---
void storage_load_zones() {
    char *data = read_file_to_string("data/zones.json");
    if (!data) return;
    printf("\n[DEBUG] Raw File Content: %s\n", data);
    cJSON *root = cJSON_Parse(data);
    if (!root) { free(data); return; }

    z_count = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, root) {
        if (z_count >= MAX_ZONES) break;

        zones[z_count].id   = safe_get_int(cJSON_GetObjectItem(item, "ID"), 0);
        zones[z_count].gpio = safe_get_int(cJSON_GetObjectItem(item, "GPIO"), 0);
        safe_strncpy(zones[z_count].name, cJSON_GetObjectItem(item, "Name"), STR_MEDIUM);
        safe_strncpy(zones[z_count].type, cJSON_GetObjectItem(item, "Type"), STR_SMALL);
        
        zones[z_count].alert_sent = false;
        z_count++;
    }
    cJSON_Delete(root);
    free(data);
}

// --- LOAD USERS ---
void storage_load_users() {
    char *data = read_file_to_string("data/users.json");
    if (!data) return;
    printf("\n[DEBUG] Raw File Content: %s\n", data);
    cJSON *root = cJSON_Parse(data);
    if (!root) { free(data); return; }

    u_count = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, root) {
        if (u_count >= MAX_USERS) break;
        safe_strncpy(users[u_count].name,  cJSON_GetObjectItem(item, "Name"),  STR_MEDIUM);
        safe_strncpy(users[u_count].pin,   cJSON_GetObjectItem(item, "PIN"),   STR_SMALL);
        u_count++;
    }
    cJSON_Delete(root);
    free(data);
}

// --- LOAD RELAYS ---
void storage_load_relays() {
    char *data = read_file_to_string("data/relays.json");
    if (!data) {
        // Fallback mock relay so Web UI isn't empty
        r_count = 1;
        relays[0].id = 1;
        relays[0].gpio = 26;
        strcpy(relays[0].name, "Main Siren");
        return;
    }
    printf("\n[DEBUG] Raw File Content: %s\n", data);
    cJSON *root = cJSON_Parse(data);
    if (!root) { free(data); return; }

    r_count = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, root) {
        if (r_count >= MAX_RELAYS) break;
        relays[r_count].id   = safe_get_int(cJSON_GetObjectItem(item, "ID"), 0);
        relays[r_count].gpio = safe_get_int(cJSON_GetObjectItem(item, "GPIO"), 0);
        safe_strncpy(relays[r_count].name, cJSON_GetObjectItem(item, "Name"), STR_MEDIUM);
        r_count++;
    }
    cJSON_Delete(root);
    free(data);
}

void storage_load_all() {
    storage_load_config();
    storage_load_zones();
    storage_load_users();
    storage_load_relays();
    printf("[STORAGE] Boot Load Complete. Zones: %d, Relays: %d\n", z_count, r_count);
}