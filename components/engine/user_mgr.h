#ifndef USER_MGR_H
#define USER_MGR_H

#include <stdbool.h>

#define MAX_USERS 10
#define STR_SMALL 64
#define STR_MEDIUM 128


typedef struct {
    char name[STR_SMALL];
    char pin[STR_SMALL];
    char phone[STR_SMALL];
    char email[STR_MEDIUM];
    int  notify;    // Integer: 0=None, 1=Email, 2=Telegram, 3=Service
    bool is_admin;  // Standardized to match your storage_mgr.c
} user_t;

char *users_to_json(void);

void user_add(user_t *users, int *u_count, const char *name, const char *pin, 
              const char *phone, const char *email, int notify, bool is_admin);

void user_update(user_t *users, int u_count, const char *name, const char *new_pin, 
                 const char *new_phone, const char *new_email, int new_notify, int new_is_admin);

void user_drop(user_t *users, int *u_count, const char *name);

void user_list(user_t *users, int u_count);

#endif