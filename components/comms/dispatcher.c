#include <stdio.h>
#include <string.h>
#include <strings.h> 
#include "dispatcher.h"
#include "storage_mgr.h"
#include "user_mgr.h"
#include "smtp.h"
#include "noonlight.h"

extern config_t config; 
extern user_t users[MAX_USERS];
extern int u_count;

static int is_type(const char *val, const char *type) {
    if (!val || !type) return 0;
    return (strcasecmp(val, type) == 0);
}

void dispatcher_alert(zone_t *z) {
    
    if (!z) return;
    
    int monitoring_enabled = 0;
    const char* category = "UNKNOWN";

    // STEP 1: GATEKEEPER
    if (is_type(z->type, "fire")) {
        category = "FIRE";
        monitoring_enabled = config.is_monitor_fire;
    } 
    else if (is_type(z->type, "police")) {
        category = "POLICE";
        monitoring_enabled = config.is_monitor_police;
    } 
    else if (is_type(z->type, "medical")) {
        category = "MEDICAL";
        monitoring_enabled = config.is_monitor_medical;
    } 
    else {
        category = "OTHER";
        monitoring_enabled = config.is_monitor_other;
    }

    if (!monitoring_enabled) {
        printf("[DISPATCHER] Gatekeeper Veto: %s monitoring is disabled.\n", category);
        return; 
    }

    // STEP 2: NOTIFY (1=None, 2=Email, 3=Telegram, 4=Noonlight)
    if (config.notify == 1) {
        printf("notify = none");
        return;
    } 

    if (config.notify == 4) {
        // Now using your noonlight.h function
        noonlight_create_alarm(&config, z);
    } 
    else if (config.notify == 2) {
        // FIXED: Calling the actual function in your smtp.c
        smtp_alert_all_contacts(&config, users, u_count, z);
    }    
    // Note: Category 3 (Telegram) would go here
}

void dispatcher_cancel_alert(void) {
    if (config.notify == 4) {
        noonlight_cancel_alarm(&config, config.pin);
    } 
    else if (config.notify == 2) {
        // FIXED: Calling the actual function in your smtp.c
        smtp_send_cancellation(&config, users, u_count);
    }
}
