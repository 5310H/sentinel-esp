#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
#include "user_mgr.h"
#include "cJSON.h"

config_t config;
relay_t relays[MAX_RELAYS];
user_t users[MAX_USERS];
zone_t zones[MAX_ZONES];
int z_count = 0, r_count = 0, u_count = 0;

static void safe_strncpy(char *dest, cJSON *obj, size_t n) {
    if (obj && obj->valuestring) {
        strncpy(dest, obj->valuestring, n - 1);
        dest[n - 1] = '\0';
    } else {
        dest[0] = '\0';
    }
}

static int safe_get_int(cJSON *obj, int fallback) {
    if (!obj) return fallback;
    return (obj->type == cJSON_Number) ? obj->valueint : fallback;
}

static bool safe_get_bool(cJSON *obj) {
    return (obj && cJSON_IsTrue(obj));
}

static char* read_file(const char* filename) {
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
void storage_save_users(void) {
    cJSON *root = cJSON_CreateArray();
    if (!root) return;

    for (int i = 0; i < u_count; i++) {
        cJSON *item = cJSON_CreateObject();
        
        cJSON_AddStringToObject(item, "name", users[i].name);
        cJSON_AddStringToObject(item, "pin", users[i].pin);
        cJSON_AddStringToObject(item, "phone", users[i].phone);
        cJSON_AddStringToObject(item, "email", users[i].email);
        
        // Save notify as an integer (0, 1, 2, 3)
        cJSON_AddNumberToObject(item, "notify", users[i].notify);
        
        // Save is_admin as a true/false boolean
        cJSON_AddBoolToObject(item, "is_admin", users[i].is_admin);

        cJSON_AddItemToArray(root, item);
    }

    char *json_str = cJSON_Print(root);
    if (json_str) {
        FILE *f = fopen("data/users.json", "w");
        if (f) {
            fputs(json_str, f);
            fclose(f);
            printf("[STORAGE] users.json updated successfully (%d users).\n", u_count);
        } else {
            printf("[STORAGE] Error: Could not open data/users.json for writing!\n");
        }
        free(json_str);
    }

    cJSON_Delete(root);
}

void storage_load_all() {
    char *data;
    cJSON *root, *item;

    // 1. LOAD CONFIG.JSON
    if ((data = read_file("data/config.json"))) {
    //    printf("%s\n", data);
        root = cJSON_Parse(data);
        if (root) {
            safe_strncpy(config.account_id, cJSON_GetObjectItem(root, "account_id"), STR_SMALL);
            
            cJSON *p = cJSON_GetObjectItem(root, "pin");
            if (p && p->type == cJSON_Number) sprintf(config.pin, "%d", p->valueint);
            else safe_strncpy(config.pin, p, STR_SMALL);

            safe_strncpy(config.name, cJSON_GetObjectItem(root, "name"), STR_MEDIUM);
            safe_strncpy(config.address1, cJSON_GetObjectItem(root, "address1"), STR_MEDIUM);
            safe_strncpy(config.address2, cJSON_GetObjectItem(root, "address2"), STR_MEDIUM);
            safe_strncpy(config.city, cJSON_GetObjectItem(root, "city"), STR_SMALL);
            safe_strncpy(config.state, cJSON_GetObjectItem(root, "state"), STR_SMALL);
            safe_strncpy(config.zip_code, cJSON_GetObjectItem(root, "zip_code"), STR_SMALL);
            safe_strncpy(config.email, cJSON_GetObjectItem(root, "email"), STR_MEDIUM);
            safe_strncpy(config.phone, cJSON_GetObjectItem(root, "phone"), STR_SMALL);
            safe_strncpy(config.instructions, cJSON_GetObjectItem(root, "instructions"), STR_LARGE);

            config.latitude = (cJSON_GetObjectItem(root, "latitude")) ? cJSON_GetObjectItem(root, "latitude")->valuedouble : 0;
            config.longitude = (cJSON_GetObjectItem(root, "longitude")) ? cJSON_GetObjectItem(root, "longitude")->valuedouble : 0;
            config.accuracy = safe_get_int(cJSON_GetObjectItem(root, "accuracy"), 5);

            // Monitoring
            safe_strncpy(config.monitor_service_id, cJSON_GetObjectItem(root, "monitor_service_id"), STR_MEDIUM);
            if(strlen(config.monitor_service_id) == 0) safe_strncpy(config.monitor_service_id, cJSON_GetObjectItem(root, "monitor_service_id"), STR_MEDIUM);
            safe_strncpy(config.monitor_service_key, cJSON_GetObjectItem(root, "monitor_service_key"), STR_MEDIUM);
            safe_strncpy(config.monitoring_url, cJSON_GetObjectItem(root, "monitoring_url"), STR_MEDIUM);
            config.notify = safe_get_int(cJSON_GetObjectItem(root, "notify"), 0);

            config.is_monitor_fire = safe_get_bool(cJSON_GetObjectItem(root, "is_monitor_fire"));
            config.is_monitor_police = safe_get_bool(cJSON_GetObjectItem(root, "is_monitor_police"));
            config.is_monitor_medical = safe_get_bool(cJSON_GetObjectItem(root, "is_monito_medical"));
            config.is_monitor_other = safe_get_bool(cJSON_GetObjectItem(root, "is_monitor_other"));

            // SMTP
            safe_strncpy(config.smtp_server, cJSON_GetObjectItem(root, "smtp_server"), STR_MEDIUM);
            config.smtp_port = safe_get_int(cJSON_GetObjectItem(root, "smtp_port"), 587);
            safe_strncpy(config.smtp_user, cJSON_GetObjectItem(root, "smtp_user"), STR_MEDIUM);
            safe_strncpy(config.smtp_pass, cJSON_GetObjectItem(root, "smtp_pass"), STR_MEDIUM);

            // MQTT
            safe_strncpy(config.mqtt_server, cJSON_GetObjectItem(root, "mqtt_server"), STR_MEDIUM);
            config.mqtt_port = safe_get_int(cJSON_GetObjectItem(root, "mqtt_port"), 1883);
            safe_strncpy(config.mqtt_user, cJSON_GetObjectItem(root, "mqtt_user"), STR_SMALL);
            safe_strncpy(config.mqtt_pass, cJSON_GetObjectItem(root, "mqtt_pass"), STR_SMALL);

            // Telegram
            safe_strncpy(config.telegram_id, cJSON_GetObjectItem(root, "telegram_id"), STR_SMALL);
            safe_strncpy(config.telegram_token, cJSON_GetObjectItem(root, "telegram_token"), STR_MEDIUM);
            config.is_telegram_enabled = safe_get_bool(cJSON_GetObjectItem(root, "is_telegram_enabled"));

            // URLs
            safe_strncpy(config.nvrserver_url, cJSON_GetObjectItem(root, "nvrserver_url"), STR_MEDIUM);
            safe_strncpy(config.haintegration_url, cJSON_GetObjectItem(root, "haintegration_url"), STR_MEDIUM);

            // Timers
            config.entry_delay = safe_get_int(cJSON_GetObjectItem(root, "entry_delay"), 30);
            config.exit_delay = safe_get_int(cJSON_GetObjectItem(root, "exit_delay"), 60);
            config.cancel_delay = safe_get_int(cJSON_GetObjectItem(root, "cancel_delay"), 10);

            cJSON_Delete(root);
        }
        free(data);
    }

    // 2. LOAD RELAYS.JSON
    r_count = 0;
    if ((data = read_file("data/relays.json"))) {
    //    printf("%s\n", data);
        root = cJSON_Parse(data);
        cJSON_ArrayForEach(item, root) {
            if (r_count >= MAX_RELAYS) break;
            relays[r_count].id = safe_get_int(cJSON_GetObjectItem(item, "id"), 0);
            safe_strncpy(relays[r_count].name, cJSON_GetObjectItem(item, "name"), STR_MEDIUM);
            safe_strncpy(relays[r_count].description, cJSON_GetObjectItem(item, "description"), STR_MEDIUM);
            relays[r_count].duration = safe_get_int(cJSON_GetObjectItem(item, "duration"), 0);
            safe_strncpy(relays[r_count].location, cJSON_GetObjectItem(item, "location"), STR_MEDIUM);
            safe_strncpy(relays[r_count].type, cJSON_GetObjectItem(item, "type"), STR_SMALL);
            relays[r_count].is_repeat = safe_get_bool(cJSON_GetObjectItem(item, "is_repeat"));
            relays[r_count].gpio = safe_get_int(cJSON_GetObjectItem(item, "gpio"), 0);
            r_count++;
        }
        if(root) cJSON_Delete(root);
        free(data);
    }

    // 3. LOAD USERS.JSON
    u_count = 0;
    if ((data = read_file("data/users.json"))) {
        printf("%s\n", data);
        root = cJSON_Parse(data);
        cJSON_ArrayForEach(item, root) {
            if (u_count >= MAX_USERS) break;
            safe_strncpy(users[u_count].name, cJSON_GetObjectItem(item, "name"), STR_SMALL);
            safe_strncpy(users[u_count].pin, cJSON_GetObjectItem(item, "pin"), STR_SMALL);
            safe_strncpy(users[u_count].phone, cJSON_GetObjectItem(item, "phone"), STR_SMALL);
            safe_strncpy(users[u_count].email, cJSON_GetObjectItem(item, "email"), STR_MEDIUM);
            users[u_count].notify = safe_get_int(cJSON_GetObjectItem(item, "notify"), 0);
            users[u_count].is_admin = safe_get_bool(cJSON_GetObjectItem(item, "is_admin"));

            u_count++;
        }
        if(root) cJSON_Delete(root);
        free(data);
    }

    // 4. LOAD ZONES.JSON
    z_count = 0;
    if ((data = read_file("data/zones.json"))) {
    //    printf("%s\n", data);
        root = cJSON_Parse(data);
        cJSON_ArrayForEach(item, root) {
            if (z_count >= MAX_ZONES) break;
            zones[z_count].id = safe_get_int(cJSON_GetObjectItem(item, "id"), 0);
            safe_strncpy(zones[z_count].name, cJSON_GetObjectItem(item, "name"), STR_SMALL);
            safe_strncpy(zones[z_count].description, cJSON_GetObjectItem(item, "description"), STR_MEDIUM);
            safe_strncpy(zones[z_count].type, cJSON_GetObjectItem(item, "type"), STR_SMALL);
            safe_strncpy(zones[z_count].location, cJSON_GetObjectItem(item, "location"), STR_SMALL);
            safe_strncpy(zones[z_count].model, cJSON_GetObjectItem(item, "model"), STR_SMALL);
            safe_strncpy(zones[z_count].manufacturer, cJSON_GetObjectItem(item, "manufacturer"), STR_SMALL);

            zones[z_count].is_chime = safe_get_bool(cJSON_GetObjectItem(item, "is_chime"));
            zones[z_count].is_alarm_on_armed_only = safe_get_bool(cJSON_GetObjectItem(item, "is_alarm_on_armed_only"));
            zones[z_count].gpio = safe_get_int(cJSON_GetObjectItem(item, "gpio"), 0);
            
            zones[z_count].is_i2c = safe_get_bool(cJSON_GetObjectItem(item, "is_i2c")) || 
                                   safe_get_bool(cJSON_GetObjectItem(item, "is_i2c"));
            zones[z_count].i2c_address = safe_get_int(cJSON_GetObjectItem(item, "i2c_address"), 0);

            zones[z_count].is_perimeter = safe_get_bool(cJSON_GetObjectItem(item, "is_perimeter")) || 
                                         safe_get_bool(cJSON_GetObjectItem(item, "Is_perimeter"));
            zones[z_count].is_interior = safe_get_bool(cJSON_GetObjectItem(item, "is_interior")) || 
                                        safe_get_bool(cJSON_GetObjectItem(item, "is_interior"));
            zones[z_count].is_panic = safe_get_bool(cJSON_GetObjectItem(item, "is_panic")) || 
                                     safe_get_bool(cJSON_GetObjectItem(item, "is_panic"));

            zones[z_count].is_alert_sent = false;
            z_count++;
        }
        if(root) cJSON_Delete(root);
        free(data);
    }
    printf("[STORAGE] Full Load Success. Z:%d R:%d U:%d\n", z_count, r_count, u_count);
}
void storage_debug_print() {
    printf("\n================= SYSTEM CONFIG DEBUG =================\n");
    printf("Account ID:    [%s]\n", config.account_id);
    printf("Owner Name:    [%s]\n", config.name);
    printf("Master PIN:    [%s]\n", config.pin);
    printf("Address:       %s, %s, %s %s\n", config.address1, config.city, config.state, config.zip_code);
    printf("Location:      Lat %.4f, Lon %.4f\n", config.latitude, config.longitude);
    printf("Notify:        [%d]\n", config.notify);
    printf("Delays:        Exit %ds | Entry %ds | Cancel %ds\n", 
            config.exit_delay, config.entry_delay, config.cancel_delay);
    
    printf("\n--- NOTIFICATIONS ---\n");
    printf("Telegram:      %s | ID: [%s]\n", 
            config.is_telegram_enabled ? "ON" : "OFF", config.telegram_id);
    printf("SMTP Server:   %s:%d | User: %s\n", 
            config.smtp_server, config.smtp_port, config.smtp_user);
    printf("MQTT Server:   %s:%d\n", config.mqtt_server, config.mqtt_port);
 
    printf("\n--- AUTHORIZED USERS (%d LOADED) ---\n", u_count);
    if (u_count == 0) {
        printf(" [!] No users found in users.json\n");
    } else {
        printf(" %-15s | %-6s | %-12s |%-17s | %s\n", "NAME", "PIN", "PHONE", " NOTIFY", "EMAIL"                 "NOTIFY");
        printf(" -----------------------------------------------------------------------------------------------\n");
        for(int i = 0; i < u_count; i++) {
            printf(" %-15s | %-6s | %-12s | %-15d  |%s\n", 
                users[i].name, 
                users[i].pin, 
                users[i].phone, 
                users[i].notify,
                users[i].email);
        }
    }

    printf("\n--- ZONES (%d LOADED) ---\n", z_count);
    for(int i = 0; i < z_count; i++) {
        printf("ID %d: %-12s | GPIO: %-2d | Type: %-8s | Chime: %s | Perim: %s | I2C: %s (0x%X)\n",
            zones[i].id, 
            zones[i].name, 
            zones[i].gpio, 
            zones[i].type,
            zones[i].is_chime ? "YES" : "NO",
            zones[i].is_perimeter ? "YES" : "NO",
            zones[i].is_i2c ? "YES" : "NO",
            zones[i].i2c_address);
    }

    printf("\n--- RELAYS (%d LOADED) ---\n", r_count);
    for(int i = 0; i < r_count; i++) {
        printf("Relay %d: %-12s | GPIO: %-2d | Type: %-8s | Repeat: %s\n", 
            relays[i].id, relays[i].name, relays[i].gpio, relays[i].type, 
            relays[i].is_repeat ? "YES" : "NO");
    }
    printf("=======================================================\n\n");
}