#include "hal.h"
#include <stdio.h>
#include <string.h>

// --- Relay Control ---
// Maps to the physical relays on the KC868-A8
void hal_set_relay(int gpio_pin, bool state) {
    if (gpio_pin <= 0) {
        printf("[HAL] INFO: Attempted to toggle invalid GPIO (%d). Ignoring.\n", gpio_pin);
        return;
    }
    
    // In Linux, we just print the state change.
    printf("[HAL] [RELAY] GPIO %d is now %s\n", 
           gpio_pin, 
           state ? "\033[0;31mCLOSED (ON)\033[0m" : "\033[0;32mOPEN (OFF)\033[0m");
}

// --- Feedback ---
void hal_buzzer_beep(int duration_ms) {
    // Simulates the keypad or internal buzzer
    printf("[HAL] [BUZZER] *BEEP* (%dms)\n", duration_ms);
}

// --- Sensor Input ---
// Accepts the full Zone_t pointer so we can log the name of the tripped zone
bool hal_get_zone_state(Zone_t *zone) {
    if (!zone) return false;

    // This is a mock: In a real system, you'd read a GPIO.
    // For the simulation, we assume the zone is SECURE (false).
    // The engine_process_rules call in main_mock manually simulates a trip.
    return false; 
}

// --- Keypad / Display Simulation ---
// Added this as a helper for your User CRUD testing
void hal_display_message(const char* msg) {
    printf("[HAL] [DISPLAY] %s\n", msg);
}