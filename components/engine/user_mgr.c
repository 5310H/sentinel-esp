#include "user_mgr.h"
#include "notifier.h"
#include <string.h>
#include <stdio.h>

static User_t* _users_ptr = NULL;
static int* _count_ptr = NULL;

void user_mgr_init(User_t* users, int* count) {
    _users_ptr = users;
    _count_ptr = count;
}

bool user_save(int id, const char* name, const char* pin, bool isAdmin) {
    if (!_users_ptr || !_count_ptr) return false;
    
    int idx = -1;
    for (int i = 0; i < *_count_ptr; i++) {
        if (_users_ptr[i].ID == id) { idx = i; break; }
    }

    if (idx == -1) {
        if (*_count_ptr >= MAX_USERS) return false;
        idx = (*_count_ptr)++;
    }

    _users_ptr[idx].ID = id;
    _users_ptr[idx].IsAdmin = isAdmin;
    strncpy(_users_ptr[idx].Name, name, sizeof(_users_ptr[idx].Name) - 1);
    strncpy(_users_ptr[idx].PIN, pin, sizeof(_users_ptr[idx].PIN) - 1);

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "USER_MOD: Added/Updated user %s (ID: %d)", name, id);
    notifier_send(NOTIFY_USER_ACTION, log_msg);

    return true;
}

bool user_drop(int id) {
    if (!_users_ptr || !_count_ptr) return false;
    int idx = -1;
    char name_copy[32] = "Unknown";

    for (int i = 0; i < *_count_ptr; i++) {
        if (_users_ptr[i].ID == id) { 
            idx = i; 
            strncpy(name_copy, _users_ptr[i].Name, 31);
            break; 
        }
    }
    if (idx == -1) return false;

    for (int i = idx; i < (*_count_ptr) - 1; i++) {
        _users_ptr[i] = _users_ptr[i + 1];
    }
    (*_count_ptr)--;

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "USER_MOD: Dropped user %s (ID: %d)", name_copy, id);
    notifier_send(NOTIFY_USER_ACTION, log_msg);

    return true;
}