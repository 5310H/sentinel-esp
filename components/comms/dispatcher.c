#include <stdio.h>
#include <string.h>
#include "dispatcher.h"
#include "noonlight.h"
#include "smtp.h"
#include "storage_mgr.h"

// External globals from storage_mgr.c
extern config_t config;
extern user_t users[];
extern int u_count;

void dispatcher_alert(zone_t *z) {
    printf("[DISPATCHER] Processing: %s\n", z->name);

    // 1. NOONLIGHT 
    if ((strcmp(z->type, "fire") == 0 && config.MonitorFire) || 
        (strcmp(z->type, "police") == 0 && config.MonitorPolice)) {
        
        noonlight_create_alarm(&config, z);
    }

    // 2. SMTP 
    // We call this directly. The SMTP function itself should handle 
    // checking if the server settings are valid/empty.
    printf("[DISPATCHER] Dispatching SMTP Alerts...\n");
    smtp_alert_all_contacts((owner_t *)&config, users, u_count, z);

    // 3. TELEGRAM (Commented out)
    /*
    if (config.TelegramEnabled) {
        // telegram_send_alert(&config, z);
    }
    */
}

void dispatcher_cancel_alert(void) {
    printf("[DISPATCHER] Cancellation Routing...\n");

    // 1. NOONLIGHT CANCEL
    if (config.MonitorFire || config.MonitorPolice) {
        noonlight_cancel_alarm(&config, config.pin);
    }

    // 2. SMTP CANCEL
    smtp_send_cancellation((owner_t *)&config, users, u_count);

    // 3. TELEGRAM CANCEL (Commented out)
    /*
    // telegram_send_status(&config, "System Disarmed");
    */
}