#ifndef HAL_H
#define HAL_H

#include <stdbool.h>
#include "config.h"

void hal_set_relay(int gpio_pin, bool state);
void hal_buzzer_beep(int duration_ms);
bool hal_get_zone_state(Zone_t *zone); // Matches the .c file
void hal_display_message(const char* msg);

#endif