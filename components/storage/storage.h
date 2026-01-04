#ifndef STORAGE_H
#define STORAGE_H

#include "config.h"

bool storage_load_all(config_t *o, User_t *u, int *uc, Zone_t *z, int *zc, Relay_t *r, int *rc, Rule_t *ru, int *ruc);

bool storage_save_all(config_t *o, User_t *u, int uc, Zone_t *z, int zc, Relay_t *r, int rc, Rule_t *ru, int ruc);

bool storage_save_owner(config_t* config);
bool storage_save_users(User_t* users, int count);

#endif
