#ifndef HAL_H
#define HAL_H

#include <stdbool.h>

// Zone inputs
bool hal_get_zone_state(int zone_id);

// Relay outputs
void hal_set_relay_state(int relay_id, bool active);
bool hal_get_relay_state(int relay_id); // Ensure this line exists

#endif