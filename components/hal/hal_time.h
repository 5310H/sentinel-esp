#ifndef HAL_TIME_H
#define HAL_TIME_H

#include <stddef.h>

/* Standardized call for the entire system */
void hal_time_init(void);
void get_sentinel_time(char *buffer, size_t len);

#endif