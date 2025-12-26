#ifndef USER_MGR_H
#define USER_MGR_H

#include <stdbool.h>
#include "config.h"

// Lifecycle: Connects the manager to the memory arrays in main
void user_mgr_init(User_t* users, int* count);

// CRUD Operations
// Save handles both "Create" (if ID is new) and "Update" (if ID exists)
bool user_save(int id, const char* name, const char* pin, bool isAdmin);

// Drop handles "Delete" by shifting the array
bool user_drop(int id);

// Debug: Prints the current user table to the console
void user_mgr_print_list(void);

#endif