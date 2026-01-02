#ifndef ENGINE_H
#define ENGINE_H

#include "storage_mgr.h"


void engine_disarm(const char* pin); // Add this line

// State Variables
extern int s_arm_state;
extern int s_chime_timer;

// Lifecycle Functions
void engine_init(void);
void engine_tick(void);
void engine_trigger_alarm(void);

// Arming Logic
void engine_arm(int state);
int engine_get_arm_state(void);

// Helper / Internal Logic
void run_security_engine(void);
void process_alarm_event(int index);

#endif