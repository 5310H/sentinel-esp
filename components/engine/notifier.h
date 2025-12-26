#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <stdbool.h>
#include "config.h"

typedef enum {
    NOTIFY_INFO,
    NOTIFY_ALARM,
    NOTIFY_SYSTEM,
    NOTIFY_USER_ACTION  // Fixes user_mgr.c error
} NotifyType_t;

void notifier_init(Owner_t* owner_ptr, User_t* users_ptr, int u_count);
void notifier_send(NotifyType_t type, const char* message);
bool notifier_cancel_alarm(void);
// Inside components/engine/notifier.h

#endif