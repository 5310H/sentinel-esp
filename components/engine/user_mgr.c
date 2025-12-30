#include "user_mgr.h"
#include "notifier.h"
#include <stdio.h>
#include <string.h>

void user_add(user_t *users, int *u_count, const char *name, const char *pin, const char *email) {
    if (*u_count < MAX_USERS) {
        strncpy(users[*u_count].name, name, 63);
        strncpy(users[*u_count].pin, pin, 7);
        strncpy(users[*u_count].email, email, 63);
        (*u_count)++;
        printf("[USER] Added: %s\n", name);
        // notifier_send is now for Alarms; logging to console instead
    }
}

void user_drop(user_t *users, int *u_count, const char *name) {
    for (int i = 0; i < *u_count; i++) {
        if (strcmp(users[i].name, name) == 0) {
            printf("[USER] Dropped: %s\n", users[i].name);
            for (int j = i; j < (*u_count) - 1; j++) {
                users[j] = users[j + 1];
            }
            (*u_count)--;
            return;
        }
    }
}
