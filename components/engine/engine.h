#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>

/**
 * @brief Initializes the engine state, timers, and arming mode.
 */
void engine_init(void);

/**
 * @brief Sets the system arming mode.
 * @param mode 0 for Disarmed, 1 for Armed (Away/Stay)
 */
void engine_arm(int mode);

/**
 * @brief Returns the current arming state of the system.
 */
int engine_get_arm_state(void);

/**
 * @brief The main logic loop. Should be called periodically (e.g., every 100ms).
 * Processes zone triggers based on:
 * - Fire: Always active, triggers alarm relays + notification.
 * - Police: Active when armed, triggers alarm relays + notification.
 * - Chime: Active when disarmed, triggers chime relays.
 */
void engine_tick(void);

/**
 * @brief Forces the alarm state into activation (used by fire/police logic).
 */
void engine_trigger_alarm(void);

/**
 * @brief Helper to check hardware relay status through the engine.
 */
bool engine_get_relay_state(int relay_id);

#endif // ENGINE_H