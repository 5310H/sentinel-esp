#ifndef STORAGE_H
#define STORAGE_H

#include "config.h"

bool storage_load_all(Owner_t *o, User_t *u, int *uc, Zone_t *z, int *zc, Relay_t *r, int *rc, Rule_t *ru, int *ruc);

bool storage_save_all(Owner_t *o, User_t *u, int uc, Zone_t *z, int zc, Relay_t *r, int rc, Rule_t *ru, int ruc);

bool storage_save_owner(Owner_t* owner);
bool storage_save_users(User_t* users, int count);

#endif
