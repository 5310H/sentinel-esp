#include <stdio.h>
#include "monitor.h"
#include "engine.h"
#include "storage_mgr.h"

// FIX: Provide the missing definition
void monitor_init(void) {
    printf("[MONITOR] Service initialized.\n");
}

void monitor_process_event(int index) {
    if (index < 0 || index >= z_count) return;
    engine_trigger_alarm(index); 
}

void monitor_scan_all() {
    extern int digitalRead(int pin);
    for (int i = 0; i < z_count; i++) {
        if (digitalRead(zones[i].gpio) == 0) {
            monitor_process_event(i);
        }
    }
}