#ifndef ENGINE_H
#define ENGINE_H

#include "config.h"

/**
 * @brief Initializes the engine with all system configurations.
 * * @param o Pointer to Owner data (Delays, Account ID)
 * @param u Pointer to User array
 * @param uc Number of users
 * @param z Pointer to Zone array
 * @param zc Number of zones
 * @param r Pointer to Relay array
 * @param rc Number of relays
 * @param ru Pointer to Rule array
 * @param ruc Number of rules
 */
void engine_init(Owner_t *o, User_t *u, int uc, Zone_t *z, int zc, Relay_t *r, int rc, Rule_t *ru, int ruc);

// State Control
void engine_arm(ArmState_t mode);
void engine_disarm(const char* input_pin);
void engine_tick(void);

// Event Handling
void engine_process_rules(int zone_id);

// Status Accessors
ArmState_t    engine_get_arm_state(void);
AlarmStatus_t engine_get_status(void);

#endif