#include <stdio.h>
#include <string.h>
#include "config.h"
#include "storage.h"
#include "engine.h"
#include "user_mgr.h"
#include "notifier.h"
#include "hal.h"

int main() {
    // 1. Setup local data structures
    Owner_t owner = {0};
    User_t  users[MAX_USERS] = {0};
    Zone_t  zones[MAX_ZONES] = {0};
    Relay_t relays[MAX_RELAYS] = {0};
    Rule_t  rules[MAX_RULES] = {0};

    int u_count = 0, z_count = 0, r_count = 0, rule_count = 0;

    printf("--- SENTINEL-ESP: FULL SYSTEM START ---\n");

    // 2. Load basic setup from JSON
    if (!storage_load_all(&owner, users, &u_count, zones, &z_count, relays, &r_count, rules, &rule_count)) {
        printf("[ERROR] Failed to load data/setup.json. Ensure file exists.\n");
        return 1;
    }

    // 3. User CRUD Setup
    // Connect manager to the RAM array and inject our test user
    user_mgr_init(users, &u_count);
    user_save(1, "Kevin", "5555", true); // ID: 1, Name: Kevin, PIN: 5555, Admin: Yes

    // 4. Manual Zone Setup for Panic Test
    // Adding a Panic Button on Zone 99
    zones[z_count].ID = 99;
    zones[z_count].PinNumber = 99;
    strncpy(zones[z_count].Name, "Master Panic", 31);
    z_count++;

    // 5. Initialize Engine
    engine_init(&owner, users, u_count, zones, z_count, relays, r_count, rules, rule_count);

    printf("\n--- STARTING SIMULATION (CRUD & PANIC TEST) ---\n");
    
    // Scenario A: Standard Away Arming
    engine_arm(ARMSTATE_AWAY);

    for (int sec = 0; sec <= 40; sec++) {
        // At 5 seconds: Trigger standard door (Zone 2)
        if (sec == 5) {
            printf("\n[EVENT] Front Door (Zone 2) Tripped!\n");
            engine_process_rules(2);
        }

        // At 15 seconds: Correct PIN entered by Kevin
        if (sec == 15) {
            printf("\n[ACTION] Kevin enters PIN '5555'...\n");
            engine_disarm("5555");
        }

        // Scenario B: System is DISARMED, but Panic is pressed
        if (sec == 25) {
            printf("\n[EVENT] Panic Button (Zone 99) Pressed while Disarmed!\n");
            engine_process_rules(99);
        }

        // At 35 seconds: Kevin resets the system after panic
        if (sec == 35) {
            printf("\n[ACTION] Resetting system with PIN '5555'...\n");
            engine_disarm("5555");
        }

        engine_tick();
    }

    printf("\n--- SIMULATION COMPLETE ---\n");
    return 0;
}