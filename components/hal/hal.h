#ifndef HAL_H
#define HAL_H

#include <stdbool.h>

void hal_set_relay(int relay_id, bool state);
bool hal_get_zone_state(int zone_id);
void hal_set_siren(bool state);
void hal_set_strobe(bool state);
bool hal_get_relay_state(int relay_id);

#endif
