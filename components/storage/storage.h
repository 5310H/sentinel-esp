#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include "config.h"

// Load Functions
bool storage_load_all(Owner_t *o, User_t *u, int *uc, Zone_t *z, int *zc, Relay_t *r, int *rc, Rule_t *ru, int *ruc);
bool storage_load_owner(Owner_t *o);
bool storage_load_users(User_t *u, int *uc);
bool storage_load_zones(Zone_t *z, int *zc);
bool storage_load_relays(Relay_t *r, int *rc);
bool storage_load_rules(Rule_t *ru, int *ruc);

// Save Functions
bool storage_save_users(User_t* users, int count);
bool storage_save_owner(Owner_t* owner);

#endif