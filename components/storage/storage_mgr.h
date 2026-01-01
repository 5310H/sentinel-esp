#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include <stdbool.h>

// String Length Constants
#define STR_SMALL  16
#define STR_MEDIUM 64
#define STR_LARGE  128

// Array Size Limits
#define MAX_ZONES  32
#define MAX_RELAYS 8
#define MAX_USERS  10

typedef struct {
    char email[STR_MEDIUM];
    char notify[STR_SMALL]; // "none", "email", "telegram", "service"
    int pin;
    // Fields for smtp.c
    char smtp_server[STR_MEDIUM];
    char smtp_user[STR_MEDIUM];
    char smtp_pass[STR_MEDIUM];
    int smtp_port;
} owner_t;

typedef struct {
    int id;
    char name[STR_MEDIUM];
    char type[STR_SMALL]; // "fire", "police", "chime"
    int relay_id;         // --- ADDED: Maps zone to hardware relay ---
    bool alert_sent;      // --- ADDED: Runtime flag to prevent spam ---
} zone_t;

typedef struct {
    int id;
    char type[STR_SMALL]; // "alarm" or "chime"
} relay_t;

typedef struct {
    char name[STR_MEDIUM];
    char pin[STR_SMALL];
    char email[STR_MEDIUM]; 
    char phone[STR_SMALL];  
} user_t;

// Function Prototypes
void storage_load_all(owner_t *o, zone_t *z, int *zc, user_t *u, int *uc, relay_t *r, int *rc);

// SHARED DATA 
extern owner_t owner;
extern zone_t zones[MAX_ZONES];
extern user_t users[MAX_USERS];
extern relay_t relays[MAX_RELAYS];
extern int z_count;
extern int u_count;
extern int r_count;

#endif