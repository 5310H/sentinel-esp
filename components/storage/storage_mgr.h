#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include <stdbool.h>

// 1. CONSTANTS FIRST (Crucial for the errors you saw)
#define STR_SMALL  32
#define STR_MEDIUM 64
#define STR_LARGE  256

#define MAX_ZONES  32
#define MAX_RELAYS 8
#define MAX_USERS  10

// 2. STRUCTURE DEFINITIONS
typedef struct {
    char name[STR_MEDIUM];
    char email[STR_MEDIUM];
    char pin[STR_SMALL];
    char phone[STR_SMALL];
    char AccountID[STR_SMALL];

    double Latitude;
    double Longitude;
    int Accuracy;
    char street[STR_MEDIUM];
    char city[STR_SMALL];
    char state[STR_SMALL];
    char zip[STR_SMALL];
    char instructions[STR_LARGE];

    char monitoringURL[STR_MEDIUM];
    char MonitorServiceKey[STR_LARGE];
// Monitoring Flags
    bool MonitorFire;      // Match exactly: MonitorFire
    bool MonitorPolice;    // Match exactly: MonitorPolice
    bool MonitorMedical;
    bool MonitorOther;
    
    char Notify[STR_SMALL];
    int EntryDelay;
    int ExitDelay;
    int CancelDelay;
    int SirenTimeout;
    int DebounceTime;
    int HeartbeatInterval;

    char TelegramID[STR_SMALL];
    char TelegramToken[STR_MEDIUM];
    bool TelegramEnabled;

    char smtp_server[STR_MEDIUM];
    char smtp_user[STR_MEDIUM];
    char smtp_pass[STR_MEDIUM];
    int smtp_port;
} config_t;

typedef config_t owner_t; // Support for dispatcher.c

typedef struct {
    int id;
    int gpio;
    char name[STR_MEDIUM];
    char description[STR_LARGE]; 
    char type[STR_SMALL];
    char mfg[STR_SMALL];
    char model[STR_SMALL];
    int relay_id;
    bool chime;                  
    bool alarm_on_armed_only;    
    bool is_perimeter;           
    bool is_interior;            
    bool is_panic;               
    bool alert_sent;             
} zone_t;

typedef struct {
    int id;
    int gpio;
    char name[STR_MEDIUM];
    char description[STR_MEDIUM];
    char type[STR_SMALL];
    bool active_high;             
} relay_t;

typedef struct {
    char name[STR_MEDIUM];
    char pin[STR_SMALL];
    char email[STR_MEDIUM];
    char phone[STR_SMALL];
} user_t;

// 3. GLOBAL EXTERN DECLARATIONS
extern config_t config;
#define owner config

extern zone_t zones[MAX_ZONES];
extern user_t users[MAX_USERS];
extern relay_t relays[MAX_RELAYS];
extern int z_count, u_count, r_count;

void storage_load_all(void);

#endif