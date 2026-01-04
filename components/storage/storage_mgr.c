#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
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

void storage_load_all() {
    char *data;
    cJSON *root, *item;

    // 1. LOAD CONFIG.JSON
    if ((data = read_file("data/config.json"))) {
    //    printf("%s\n", data);
        root = cJSON_Parse(data);
        if (root) {
            safe_strncpy(config.accountid, cJSON_GetObjectItem(root, "accountid"), STR_SMALL);
            
            cJSON *p = cJSON_GetObjectItem(root, "pin");
            if (p && p->type == cJSON_Number) sprintf(config.pin, "%d", p->valueint);
            else safe_strncpy(config.pin, p, STR_SMALL);

            safe_strncpy(config.name, cJSON_GetObjectItem(root, "name"), STR_MEDIUM);
            safe_strncpy(config.address1, cJSON_GetObjectItem(root, "address1"), STR_MEDIUM);
            safe_strncpy(config.address2, cJSON_GetObjectItem(root, "address2"), STR_MEDIUM);
            safe_strncpy(config.city, cJSON_GetObjectItem(root, "city"), STR_SMALL);
            safe_strncpy(config.state, cJSON_GetObjectItem(root, "state"), STR_SMALL);
            safe_strncpy(config.zipcode, cJSON_GetObjectItem(root, "zipcode"), STR_SMALL);
            safe_strncpy(config.email, cJSON_GetObjectItem(root, "email"), STR_MEDIUM);
            safe_strncpy(config.phone, cJSON_GetObjectItem(root, "phone"), STR_SMALL);
            safe_strncpy(config.instructions, cJSON_GetObjectItem(root, "instructions"), STR_LARGE);

            config.latitude = (cJSON_GetObjectItem(root, "latitude")) ? cJSON_GetObjectItem(root, "latitude")->valuedouble : 0;
            config.longitude = (cJSON_GetObjectItem(root, "longitude")) ? cJSON_GetObjectItem(root, "longitude")->valuedouble : 0;
            config.accuracy = safe_get_int(cJSON_GetObjectItem(root, "accuracy"), 5);

            // Monitoring
            safe_strncpy(config.monitorserviceid, cJSON_GetObjectItem(root, "monitorserviceid"), STR_MEDIUM);
            if(strlen(config.monitorserviceid) == 0) safe_strncpy(config.monitorserviceid, cJSON_GetObjectItem(root, "monitorserviceif"), STR_MEDIUM);
            safe_strncpy(config.monitorservicekey, cJSON_GetObjectItem(root, "monitorservicekey"), STR_MEDIUM);
            safe_strncpy(config.monitoringurl, cJSON_GetObjectItem(root, "monitoringurl"), STR_MEDIUM);
            safe_strncpy(config.notify, cJSON_GetObjectItem(root, "notify"), STR_SMALL);

            config.monitorfire = safe_get_bool(cJSON_GetObjectItem(root, "monitorfire"));
            config.monitorpolice = safe_get_bool(cJSON_GetObjectItem(root, "monitorpolice"));
            config.monitormedical = safe_get_bool(cJSON_GetObjectItem(root, "monitormedical"));
            config.monitorother = safe_get_bool(cJSON_GetObjectItem(root, "monitorother"));

            // SMTP
            safe_strncpy(config.smtpserver, cJSON_GetObjectItem(root, "smtpserver"), STR_MEDIUM);
            config.smtpport = safe_get_int(cJSON_GetObjectItem(root, "smtpport"), 587);
            safe_strncpy(config.smtpuser, cJSON_GetObjectItem(root, "smtpuser"), STR_MEDIUM);
            safe_strncpy(config.smtppass, cJSON_GetObjectItem(root, "smtppass"), STR_MEDIUM);

            // MQTT
            safe_strncpy(config.mqttserver, cJSON_GetObjectItem(root, "mqttserver"), STR_MEDIUM);
            config.mqttport = safe_get_int(cJSON_GetObjectItem(root, "mqttport"), 1883);
            safe_strncpy(config.mqttuser, cJSON_GetObjectItem(root, "mqttuser"), STR_SMALL);
            safe_strncpy(config.mqttpass, cJSON_GetObjectItem(root, "mqttpass"), STR_SMALL);

            // Telegram
            safe_strncpy(config.telegramid, cJSON_GetObjectItem(root, "telegramid"), STR_SMALL);
            safe_strncpy(config.telegramtoken, cJSON_GetObjectItem(root, "telegramtoken"), STR_MEDIUM);
            config.istelegramenabled = safe_get_bool(cJSON_GetObjectItem(root, "istelegramenabled"));

            // URLs
            safe_strncpy(config.nvrserverURL, cJSON_GetObjectItem(root, "nvrserverURL"), STR_MEDIUM);
            safe_strncpy(config.haintegrationURL, cJSON_GetObjectItem(root, "haintegrationURL"), STR_MEDIUM);

            // Timers
            config.entrydelay = safe_get_int(cJSON_GetObjectItem(root, "entrydelay"), 30);
            config.exitdelay = safe_get_int(cJSON_GetObjectItem(root, "exitdelay"), 60);
            config.canceldelay = safe_get_int(cJSON_GetObjectItem(root, "canceldelay"), 10);

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
            relays[r_count].durationSec = safe_get_int(cJSON_GetObjectItem(item, "durationSec"), 0);
            safe_strncpy(relays[r_count].location, cJSON_GetObjectItem(item, "location"), STR_MEDIUM);
            safe_strncpy(relays[r_count].type, cJSON_GetObjectItem(item, "type"), STR_SMALL);
            relays[r_count].isrepeat = safe_get_bool(cJSON_GetObjectItem(item, "isrepeat"));
            relays[r_count].gpio = safe_get_int(cJSON_GetObjectItem(item, "gpio"), 0);
            r_count++;
        }
        if(root) cJSON_Delete(root);
        free(data);
    }

    // 3. LOAD USERS.JSON
    u_count = 0;
    if ((data = read_file("data/users.json"))) {
    //  printf("%s\n", data);
        root = cJSON_Parse(data);
        cJSON_ArrayForEach(item, root) {
            if (u_count >= MAX_USERS) break;
            safe_strncpy(users[u_count].name, cJSON_GetObjectItem(item, "name"), STR_SMALL);
            safe_strncpy(users[u_count].pin, cJSON_GetObjectItem(item, "pin"), STR_SMALL);
            safe_strncpy(users[u_count].phone, cJSON_GetObjectItem(item, "phone"), STR_SMALL);
            safe_strncpy(users[u_count].email, cJSON_GetObjectItem(item, "email"), STR_MEDIUM);
            safe_strncpy(users[u_count].notify, cJSON_GetObjectItem(item, "notify"), STR_SMALL);
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

            zones[z_count].ischime = safe_get_bool(cJSON_GetObjectItem(item, "isChime"));
            zones[z_count].isalarmonarmedonly = safe_get_bool(cJSON_GetObjectItem(item, "isAlarmOnArmedOnly"));
            zones[z_count].gpio = safe_get_int(cJSON_GetObjectItem(item, "gpio"), 0);
            
            zones[z_count].isi2c = safe_get_bool(cJSON_GetObjectItem(item, "isI2C")) || 
                                   safe_get_bool(cJSON_GetObjectItem(item, "sI2C"));
            zones[z_count].i2caddress = safe_get_int(cJSON_GetObjectItem(item, "I2CAddress"), 0);

            zones[z_count].isperimeter = safe_get_bool(cJSON_GetObjectItem(item, "isPerimeter")) || 
                                         safe_get_bool(cJSON_GetObjectItem(item, "IsPerimeter"));
            zones[z_count].isinterior = safe_get_bool(cJSON_GetObjectItem(item, "isInterior")) || 
                                        safe_get_bool(cJSON_GetObjectItem(item, "IsInterior"));
            zones[z_count].ispanic = safe_get_bool(cJSON_GetObjectItem(item, "isPanic")) || 
                                     safe_get_bool(cJSON_GetObjectItem(item, "IsPanic"));

            zones[z_count].alert_sent = false;
            z_count++;
        }
        if(root) cJSON_Delete(root);
        free(data);
    }
    printf("[STORAGE] Full Load Success. Z:%d R:%d U:%d\n", z_count, r_count, u_count);
}
void storage_debug_print() {
    printf("\n================= SYSTEM CONFIG DEBUG =================\n");
    printf("Account ID:    [%s]\n", config.accountid);
    printf("Owner Name:    [%s]\n", config.name);
    printf("Master PIN:    [%s]\n", config.pin);
    printf("Address:       %s, %s, %s %s\n", config.address1, config.city, config.state, config.zipcode);
    printf("Location:      Lat %.4f, Lon %.4f\n", config.latitude, config.longitude);
    printf("Notify:        [%s]\n", config.notify);
    printf("Delays:        Exit %ds | Entry %ds | Cancel %ds\n", 
            config.exitdelay, config.entrydelay, config.canceldelay);
    
    printf("\n--- NOTIFICATIONS ---\n");
    printf("Telegram:      %s | ID: [%s]\n", 
            config.istelegramenabled ? "ON" : "OFF", config.telegramid);
    printf("SMTP Server:   %s:%d | User: %s\n", 
            config.smtpserver, config.smtpport, config.smtpuser);
    printf("MQTT Server:   %s:%d\n", config.mqttserver, config.mqttport);
 
    printf("\n--- AUTHORIZED USERS (%d LOADED) ---\n", u_count);
    if (u_count == 0) {
        printf(" [!] No users found in users.json\n");
    } else {
        printf(" %-15s | %-6s | %-12s |%-17s | %s\n", "NAME", "PIN", "PHONE", " NOTIFY", "EMAIL"                 "NOTIFY");
        printf(" -----------------------------------------------------------------------------------------------\n");
        for(int i = 0; i < u_count; i++) {
            printf(" %-15s | %-6s | %-12s | %-15s  |%s\n", 
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
            zones[i].ischime ? "YES" : "NO",
            zones[i].isperimeter ? "YES" : "NO",
            zones[i].isi2c ? "YES" : "NO",
            zones[i].i2caddress);
    }

    printf("\n--- RELAYS (%d LOADED) ---\n", r_count);
    for(int i = 0; i < r_count; i++) {
        printf("Relay %d: %-12s | GPIO: %-2d | Type: %-8s | Repeat: %s\n", 
            relays[i].id, relays[i].name, relays[i].gpio, relays[i].type, 
            relays[i].isrepeat ? "YES" : "NO");
    }
    printf("=======================================================\n\n");
}