#include <stdio.h>
#include <string.h>
#include <strings.h> 
#include "dispatcher.h"
#include "storage_mgr.h"

// Reference the global config and mock flag
extern config_t config; 

/**
 * HELPER: Case-insensitive type comparison
 */
static int is_type(const char *val, const char *type) {
    if (!val || !type) return 0;
    return (strcasecmp(val, type) == 0);
}

void dispatcher_alert(zone_t *z) {
    if (!z) return;
    
    int monitoring_enabled = 0;
    const char* category = "UNKNOWN";

    // --- DEBUG OUTPUT ---
    // This confirms if the flags loaded from storage_mgr are actually 1 or 0
    printf("\n[DEBUG-DISPATCHER] ----------------------------\n");
    printf("[DEBUG-DISPATCHER] Zone Name: '%s'\n", z->name);
    printf("[DEBUG-DISPATCHER] Zone Type: '%s'\n", z->type);
    printf("[DEBUG-DISPATCHER] Flags: Fire=%d, Police=%d, Med=%d, Other=%d\n", 
            config.monitorfire, config.monitorpolice, config.monitormedical, config.monitorother);
    printf("[DEBUG-DISPATCHER] ----------------------------\n\n");

    // ==========================================================
    // STEP 1: CATEGORY GATEKEEPER
    // ==========================================================
    if (is_type(z->type, "fire")) {
        category = "FIRE";
        monitoring_enabled = config.monitorfire;
    } 
    else if (is_type(z->type, "police") || is_type(z->type, "door") || is_type(z->type, "motion")) {
        category = "POLICE";
        monitoring_enabled = config.monitorpolice;
    } 
    else if (is_type(z->type, "medical")) {
        category = "MEDICAL";
        monitoring_enabled = config.monitormedical;
    } 
    else {
        category = "OTHER";
        monitoring_enabled = config.monitorother;
    }

    // If monitoring is disabled for this category, we stop here.
    if (!monitoring_enabled) {
        printf("[DISPATCHER] ABORTED: %s monitoring is set to FALSE. No notifications sent.\n", category);
        return; 
    }

    printf("[DISPATCHER] ALERT ACTIVE: %s monitoring is ENABLED.\n", category);

    // ==========================================================
    // STEP 2: PROFESSIONAL SERVICE (NOONLIGHT)
    // ==========================================================
    // Triggered only if global notify allows "service" or "all"
    if (is_type(config.notify, "service") || is_type(config.notify, "all")) {

    }

    // ==========================================================
    // STEP 3: INDIVIDUAL USER NOTIFICATIONS (Telegram/SMTP)
    // ==========================================================
    if (is_type(config.notify, "none")) {
        printf("[DISPATCHER] -> Global Notify is 'none'. Bypassing individual users.\n");
        return;
    }

    // Logic for looping through users would go here

        printf("[MOCK] -> Would send Telegram/Email to configured users.\n");

}

/**
 * CANCEL ALERT
 * Called when the system is disarmed during an active alarm.
 */
void dispatcher_cancel_alert(void) {
    printf("[DISPATCHER] Attempting to cancel remote monitoring alerts...\n");
}