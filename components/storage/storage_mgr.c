#include "storage_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

// 1. Core loading functions
void storage_load_owner(owner_t *obj) {
    // This is a placeholder; in a real run, it parses owner.json
    printf("[STORAGE] Loading Owner configuration...\n");
    strcpy(obj->notify, "email"); 
}

void storage_load_users(user_t *list, int *count) {
    printf("[STORAGE] Loading User database...\n");
    *count = 0; // Placeholder
}

// 2. The Bridge Functions (ONLY DEFINE THESE ONCE)
void storage_load_all(owner_t *o, zone_t *z, int *zc, user_t *u, int *uc) {
    storage_load_owner(o);
    storage_load_users(u, uc);
    // Setup a default zone for the mock test
    z[0].id = 1;
    strcpy(z[0].name, "Front Door");
    *zc = 1;
    printf("[STORAGE] All data loaded into memory.\n");
}

void storage_save_all(owner_t *o, zone_t *z, int zc, user_t *u, int uc) {
    printf("[STORAGE] Mock Save: Data synced to virtual storage.\n");
}