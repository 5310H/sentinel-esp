#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include "dispatcher.h"

void storage_load_owner(owner_t *obj);
void storage_load_users(user_t *list, int max, int *count);
// Add zone loading if needed:
// void storage_load_zones(zone_t *list, int max, int *count);

#endif