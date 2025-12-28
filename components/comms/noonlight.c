#include <stdio.h>
#include "dispatcher.h"

void noonlight_trigger(owner_t *home) {
    printf("\n[NOONLIGHT] ALERT! Initiating emergency dispatch sequence...\n");
    printf("[NOONLIGHT] POST %s\n", home->monitoring_url);
    printf("[NOONLIGHT] Key: %s | Location: %f, %f\n", 
           home->monitor_service_key, home->latitude, home->longitude);
    printf("[NOONLIGHT] Dispatch Confirmed.\n");
}
