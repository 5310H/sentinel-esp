#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "config.h"
#include <stdbool.h>

typedef enum {
    NOTIFY_ALARM,
    NOTIFY_ARM_STATE,
    NOTIFY_USER_ACTION,
    NOTIFY_ERROR
} NotifyType_t;

// Lifecycle
void notifier_init(const char* url, const char* account_id);

// Outbound
void notifier_send(NotifyType_t type, const char* message);
void notifier_send_email(const char* subject, const char* body);

// Control
bool notifier_cancel_alarm(void);

#endif