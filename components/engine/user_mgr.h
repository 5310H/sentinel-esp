#ifndef USER_MGR_H
#define USER_MGR_H

#include "storage_mgr.h"
#include <stdbool.h>

void user_add(user_t *users, int *u_count, const char *name, const char *pin, const char *email);
void user_drop(user_t *users, int *u_count, const char *name);

#endif
