#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <stdbool.h>

/* --- Sizing Constants --- */
#define STR_SMALL   16
#define STR_MEDIUM  64
#define STR_LARGE   128
#define MAX_USER    8
#define MAX_ZONES   16
#define MAX_RELAYS  8
#define MAX_RULES   20

/* --- System States and Events --- */
#define STATE_DISARMED    0
#define STATE_ARMED_AWAY  1
#define STATE_ARMED_STAY  2
#define EVENT_ZONE_TRIP   10

/* ================= USER ================= */
typedef struct {
    char name[STR_MEDIUM];
    char pin[STR_SMALL];
    char phone[STR_SMALL];
    char email[STR_MEDIUM];
    char notify[STR_SMALL];
    bool is_admin;
} user_t;

/* ================= OWNER/SYSTEM ================= */
typedef struct {
    char account_id[STR_MEDIUM];
    char name[STR_MEDIUM];
    char address1[STR_LARGE];
    char address2[STR_LARGE];
    char city[STR_MEDIUM];
    char state[STR_SMALL];
    char zip[STR_SMALL];
    char phone[STR_SMALL];
    char email[STR_MEDIUM];
    char instructions[STR_LARGE * 2];
    
    double latitude;
    double longitude;
    double accuracy;

    char monitor_service_key[STR_MEDIUM];
    char monitoring_url[STR_LARGE];
    bool monitor_fire;
    bool monitor_police;
    bool monitor_medical;

    char smtp_server[STR_MEDIUM];
    int  smtp_port;
    char smtp_user[STR_MEDIUM];
    char smtp_pass[STR_MEDIUM];

    int  entry_delay;
    int  exit_delay;
    int  cancel_delay;
} owner_t;

/* ================= ZONE ================= */
typedef struct {
    int  id;
    char name[STR_MEDIUM];
    bool chime;
    int  pin_number;
} zone_t;

// Global state variable
extern int current_state;

// Prototypes
void dispatcher_init(owner_t *o, zone_t *z, int z_cnt, user_t *u, int u_cnt);
void dispatcher_process_event(int event_type, int zone_id);
void noonlight_trigger(owner_t *home);
void smtp_send_email(owner_t *home, const char* subject, const char* body);

#endif