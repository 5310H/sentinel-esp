#ifndef ENGINE_H
#define ENGINE_H

#include "storage_mgr.h" // Pulls in the authoritative zone_t, relay_t, etc.

// Initialization
void engine_init(void);

// Arming Control
void engine_arm(int mode);
int engine_get_arm_state(void);

// Main Loop Process
void engine_tick(void);

// Manual Trigger (Needed by monitor.c)
void engine_trigger_alarm(void);

#endif