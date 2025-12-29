#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#define STR_SMALL  32
#define STR_MEDIUM 128
#define MAX_USER   10

// 1. DATA STRUCTURES FIRST (No logic here)
typedef struct {
    char notify[STR_SMALL];
    char email[STR_MEDIUM];
    char smtp_server[STR_MEDIUM];
    int  smtp_port;
    char smtp_user[STR_MEDIUM];
    char smtp_pass[STR_MEDIUM];
} owner_t;

typedef struct {
    char name[STR_MEDIUM];
    char phone[STR_SMALL];
    char email[STR_MEDIUM];
} user_t;

typedef struct {
    int id;
    char name[STR_MEDIUM];
} zone_t;

// 2. FUNCTION PROTOTYPES SECOND
void storage_load_owner(owner_t *obj);
void storage_load_users(user_t *list, int *count);
void storage_load_all(owner_t *o, zone_t *z, int *zc, user_t *u, int *uc);
void storage_save_all(owner_t *o, zone_t *z, int zc, user_t *u, int uc);

#endif