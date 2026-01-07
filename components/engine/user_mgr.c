#include "user_mgr.h"
#include "storage_mgr.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>

void user_add(user_t *users, int *u_count, const char *name, const char *pin, 
              const char *phone, const char *email, int notify, bool is_admin) {
    if (*u_count < MAX_USERS) {
        user_t *u = &users[*u_count];
        
        strncpy(u->name, name, STR_SMALL - 1);
        strncpy(u->pin, pin, STR_SMALL - 1);
        strncpy(u->phone, phone, STR_SMALL - 1);
        strncpy(u->email, email, STR_MEDIUM - 1);
        u->notify = notify;   
        u->is_admin = is_admin;
        
        (*u_count)++;
        printf("[USER] Created: %s (Admin: %d, Notify: %d)\n", name, is_admin, notify);
        storage_save_users(); 
    }
}

void user_update(user_t *users, int u_count, const char *name, 
                 const char *new_pin, const char *new_phone, const char *new_email, 
                 int new_notify, int new_is_admin) {
    for (int i = 0; i < u_count; i++) {
        if (strcmp(users[i].name, name) == 0) {
            if (new_pin && strlen(new_pin) > 0) strncpy(users[i].pin, new_pin, STR_SMALL - 1);
            if (new_phone && strlen(new_phone) > 0) strncpy(users[i].phone, new_phone, STR_SMALL - 1);
            if (new_email && strlen(new_email) > 0) strncpy(users[i].email, new_email, STR_MEDIUM - 1);
            
            if (new_notify >= 0) users[i].notify = new_notify;
            if (new_is_admin != -1) users[i].is_admin = (bool)new_is_admin;

            storage_save_users();
            return;
        }
    }
}

void user_drop(user_t *users, int *u_count, const char *name) {
    for (int i = 0; i < *u_count; i++) {
        if (strcmp(users[i].name, name) == 0) {
            for (int j = i; j < (*u_count) - 1; j++) {
                users[j] = users[j + 1];
            }
            (*u_count)--;
            storage_save_users();
            return;
        }
    }
}

void user_list(user_t *users, int u_count) {
    printf("\n--- SENTINEL USER DATABASE ---\n");
    for (int i = 0; i < u_count; i++) {
        printf("[%d] %-12s | Admin: %s | Notify: %d | Email: %s\n", 
               i, users[i].name, users[i].is_admin ? "YES" : "NO ", 
               users[i].notify, users[i].email);
    }
}

char *users_to_json(void) {
    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < u_count; i++) {
        cJSON *u = cJSON_CreateObject();
        cJSON_AddStringToObject(u, "name", users[i].name);
        cJSON_AddStringToObject(u, "email", users[i].email);
        cJSON_AddNumberToObject(u, "notify", users[i].notify);
        cJSON_AddBoolToObject(u, "is_admin", users[i].is_admin);
        cJSON_AddItemToArray(root, u);
    }
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}