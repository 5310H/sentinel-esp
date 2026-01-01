#include "dispatcher.h"
#include "smtp.h"
#include "noonlight.h"
#include <stdio.h>
#include <string.h>

// External data from storage
extern owner_t owner;
extern user_t users[MAX_USERS];
extern int u_count;
extern zone_t zones[MAX_ZONES];
extern int z_count;

// --- THIS IS THE MISSING FUNCTION ---
void notifier_init(void) {
    printf("[DISPATCHER] Notification system initialized (Mode: %s)\n", owner.notify);
}

void notifier_send(int zone_id) {
    // ... (Your strcmp logic for none, email, telegram, service) ...
    // Make sure this is NOT static
}

void notifier_cancel_alarm(void) {
    // ... (Your cancel logic) ...
    // Make sure this is NOT static
}