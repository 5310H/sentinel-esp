#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include <stdbool.h>
#include "cJSON.h"
#include "user_mgr.h"

// String size definitions for memory safety
#define STR_SMALL 64
#define STR_MEDIUM 128
#define STR_LARGE 512

// Array limit definitions
#define MAX_ZONES 32
#define MAX_USERS 10
#define MAX_RELAYS 10
// Inside storage_mgr.h

extern user_t users[MAX_USERS]; // Tells other files the array exists in storage_mgr.o
extern int u_count;
typedef enum {
    NOTIFY_NONE     = 0,
    NOTIFY_EMAIL    = 1,
    NOTIFY_TELEGRAM = 2,
    NOTIFY_SERVICE  = 3
} notify_mode_t;

/**
 * System Configuration Struct
 * Maps to config.json
 */
typedef struct {
    char account_id[STR_SMALL];
    char pin[STR_SMALL];
    char name[STR_MEDIUM];
    char address1[STR_MEDIUM];
    char address2[STR_MEDIUM];
    char city[STR_SMALL];
    char state[STR_SMALL];
    char zip_code[STR_SMALL];
    char email[STR_MEDIUM];
    char phone[STR_SMALL];
    char instructions[STR_LARGE];
    
    double latitude;
    double longitude;
    int accuracy;

    // Monitoring Service (Noonlight)
    char monitor_service_id[STR_MEDIUM];
    char monitor_service_key[STR_MEDIUM];
    char monitoring_url[STR_MEDIUM];
    int notify;
    
    bool is_monitor_fire;
    bool is_monitor_police;
    bool is_monitor_medical;
    bool is_monitor_other;

    // Email (SMTP)
    char smtp_server[STR_MEDIUM];
    int smtp_port;
    char smtp_user[STR_MEDIUM];
    char smtp_pass[STR_MEDIUM];

    // MQTT
    char mqtt_server[STR_MEDIUM];
    int mqtt_port;
    char mqtt_user[STR_SMALL];
    char mqtt_pass[STR_SMALL];

    // Telegram
    char telegram_id[STR_SMALL];
    char telegram_token[STR_MEDIUM];
    bool is_telegram_enabled;

    // External Integrations
    char nvrserver_url[STR_MEDIUM];
    char haintegration_url[STR_MEDIUM];

    // Timers
    int entry_delay;
    int exit_delay;
    int cancel_delay;
} config_t;

/**
 * Relay Configuration Struct
 * Maps to relays.json
 */
typedef struct {
    int id;
    char name[STR_MEDIUM];
    char description[STR_MEDIUM];
    int duration;
    char location[STR_MEDIUM];
    char type[STR_SMALL];
    bool is_repeat;
    int gpio;
} relay_t;

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
    
    bool is_chime;
    bool is_alarm_on_armed_only;
    int gpio;
    
    // I2C Hardware Support
    bool is_i2c;
    int i2c_address;
    
    // Logic Flags
    bool is_perimeter;
    bool is_interior;
    bool is_panic;
    
    // Runtime state (Not in JSON)
    bool is_alert_sent; 
} zone_t;

// --- Global Data Stores ---
extern config_t config;
extern relay_t relays[MAX_RELAYS];
//extern user_t users[MAX_USERS];
extern zone_t zones[MAX_ZONES];

extern int z_count;
extern int r_count;
extern int u_count;

// --- Functions ---
void storage_load_all();
void storage_debug_print();
void storage_save_users(void);
#endif