#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include <stdbool.h>

// String size definitions for memory safety
#define STR_SMALL 64
#define STR_MEDIUM 128
#define STR_LARGE 512

// Array limit definitions
#define MAX_ZONES 32
#define MAX_USERS 10
#define MAX_RELAYS 10

/**
 * System Configuration Struct
 * Maps to config.json
 */
typedef struct {
    char accountid[STR_SMALL];
    char pin[STR_SMALL];
    char name[STR_MEDIUM];
    char address1[STR_MEDIUM];
    char address2[STR_MEDIUM];
    char city[STR_SMALL];
    char state[STR_SMALL];
    char zipcode[STR_SMALL];
    char email[STR_MEDIUM];
    char phone[STR_SMALL];
    char instructions[STR_LARGE];
    
    double latitude;
    double longitude;
    int accuracy;

    // Monitoring Service (Noonlight)
    char monitorserviceid[STR_MEDIUM];
    char monitorservicekey[STR_MEDIUM];
    char monitoringurl[STR_MEDIUM];
    char notify[STR_SMALL];
    
    bool monitorfire;
    bool monitorpolice;
    bool monitormedical;
    bool monitorother;

    // Email (SMTP)
    char smtpserver[STR_MEDIUM];
    int smtpport;
    char smtpuser[STR_MEDIUM];
    char smtppass[STR_MEDIUM];

    // MQTT
    char mqttserver[STR_MEDIUM];
    int mqttport;
    char mqttuser[STR_SMALL];
    char mqttpass[STR_SMALL];

    // Telegram
    char telegramid[STR_SMALL];
    char telegramtoken[STR_MEDIUM];
    bool istelegramenabled;

    // External Integrations
    char nvrserverURL[STR_MEDIUM];
    char haintegrationURL[STR_MEDIUM];

    // Timers
    int entrydelay;
    int exitdelay;
    int canceldelay;
} config_t;

/**
 * Relay Configuration Struct
 * Maps to relays.json
 */
typedef struct {
    int id;
    char name[STR_MEDIUM];
    char description[STR_MEDIUM];
    int durationSec;
    char location[STR_MEDIUM];
    char type[STR_SMALL];
    bool isrepeat;
    int gpio;
} relay_t;

/**
 * User Configuration Struct
 * Maps to users.json
 */
typedef struct {
    char name[STR_SMALL];
    char pin[STR_SMALL];
    char phone[STR_SMALL];
    char email[STR_MEDIUM];
    char notify[STR_SMALL];
} user_t;

/**
 * Zone Configuration Struct
 * Maps to zones.json
 */
typedef struct {
    int id;
    char name[STR_SMALL];
    char description[STR_MEDIUM];
    char type[STR_SMALL];
    char location[STR_SMALL];
    char model[STR_SMALL];
    char manufacturer[STR_SMALL];
    
    bool ischime;
    bool isalarmonarmedonly;
    int gpio;
    
    // I2C Hardware Support
    bool isi2c;
    int i2caddress;
    
    // Logic Flags
    bool isperimeter;
    bool isinterior;
    bool ispanic;
    
    // Runtime state (Not in JSON)
    bool alert_sent; 
} zone_t;

// --- Global Data Stores ---
extern config_t config;
extern relay_t relays[MAX_RELAYS];
extern user_t users[MAX_USERS];
extern zone_t zones[MAX_ZONES];

extern int z_count;
extern int r_count;
extern int u_count;

// --- Functions ---
void storage_load_all();
void storage_debug_print();

#endif