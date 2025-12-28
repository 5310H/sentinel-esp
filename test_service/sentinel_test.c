#include <stdio.h>
#include <curl/curl.h>
#include "dispatcher.h"
#include "storage_mgr.h"

int main() {
    owner_t home = {0};
    user_t users[MAX_USER] = {0};
    zone_t zones[MAX_ZONES] = {0};
    int u_cnt = 0, z_cnt = 0;

    // Initialize Global Networking
    curl_global_init(CURL_GLOBAL_ALL);
    
    printf("--- STARTING SENTINEL TRUE TEST ---\n");

    // 1. Load the Data from JSON
    storage_load_owner(&home);
//    storage_load_users(users, MAX_USER, &u_cnt);

    // 2. Initialize the Logic Engine
    dispatcher_init(&home, zones, z_cnt, users, u_cnt);

    // 3. Simulate an Arming Event
    printf("[TEST] Setting state to ARMED_AWAY...\n");
    current_state = STATE_ARMED_AWAY;
    
    // 4. Simulate a Zone Trip
    printf("[TEST] Triggering Zone Trip Event...\n");
    dispatcher_process_event(EVENT_ZONE_TRIP, 1);

    printf("--- TEST SEQUENCE FINISHED ---\n");

    curl_global_cleanup();
    return 0;
}