#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>
#include "storage_mgr.h"

// --- Enumerations ---

/**
 * Arming Modes
 * AWAY: All zones active
 * STAY: Interior zones bypassed, Perimeter active
 * NIGHT: Perimeter and select interior active
 */
typedef enum {
    ARM_AWAY = 0,
    ARM_STAY,
    ARM_NIGHT
} arm_mode_t;

/**
 * System States
 * 0: Disarmed
 * 1: Exit Delay (Arming in progress)
 * 2: Armed (Monitoring active)
 * 3: Entry Delay (Waiting for PIN)
 * 4: Alarmed (Siren and Alerts active)
 */
extern int s_arm_state;

// --- Structures ---

/**
 * Keypad Result
 * Returned when a PIN is entered to determine UI behavior
 */
typedef struct {
    bool authenticated;    // Valid PIN found?
    bool is_admin;         // Does user have admin rights?
    char name[STR_SMALL];  // User's name for greeting
} keypad_result_t;

// --- Core Logic Functions ---

/**
 * Initializes timers and system state
 */
void engine_init(void);

/**
 * Main loop process - handles timers and zone monitoring
 * Should be called once per second
 */
void engine_tick(void);

// --- Public Keypad Actions ---

/**
 * Validates a PIN against Master and User records
 */
keypad_result_t engine_check_keypad(const char* input_pin);

/**
 * Transitions system from Disarmed to Exit Delay
 */
void engine_ui_arm(arm_mode_t mode);

/**
 * Transitions system to Disarmed and resets hardware
 */
void engine_ui_disarm(void);

/**
 * Forces the system into an alarm state (Panic/Fire/Timeout)
 */
void engine_trigger_alarm(int trigger_index);

/**
 * Returns the current arming state (0-4)
 */
int engine_get_arm_state(void);

#endif // ENGINE_H