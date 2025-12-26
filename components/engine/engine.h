#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>
#include "config.h"

void engine_init(void);
void engine_arm(int mode);
void engine_disarm(const char* pin);
void engine_tick(void); 

// State Getters for the Web UI
int engine_get_arm_state(void);
int engine_get_status(void);
bool engine_get_relay_state(int relay_id); // New getter

void engine_trigger_alarm(void);

#endif