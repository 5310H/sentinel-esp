#ifndef ENGINE_H
#define ENGINE_H

#include "storage_mgr.h"

// --- State Variables ---
extern int s_arm_state;
extern int s_chime_timer;

// --- Lifecycle Functions ---
void engine_init(void);
void engine_tick(void);

/**
 * FIX: Changed from void to int trigger_index.
 * This resolves the "conflicting types" error in engine.c
 */
void engine_trigger_alarm(int trigger_index);

// --- Arming & Disarming Logic ---
void engine_arm(int state);
void engine_disarm(const char* pin);
int  engine_get_arm_state(void);

// --- Internal Processing ---
void process_alarm_event(int index);

#endif