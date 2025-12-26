#include <stdio.h>
#include <string.h>
#include "dispatcher.h"

static Owner_t *_owner;

void dispatcher_init(Owner_t *owner) {
    _owner = owner;
    printf("[DISPATCHER] Initialized. Target: %s\n", _owner->MonitoringURL);
}

void dispatcher_send_alarm(Zone_t *zone) {
    printf("\n--- OUTGOING DISPATCH --- \n");
    
    // 1. Mock Noonlight JSON Payload
    printf("[NOONLIGHT] POST %s\n", _owner->MonitoringURL);
    printf("[NOONLIGHT] Body: { \"service_key\": \"%s\", \"location\": {\"lat\": %f, \"lng\": %f}, \"instructions\": \"%s: %s\" }\n",
           _owner->MonitorServiceKey, _owner->Latitude, _owner->Longitude, zone->Name, zone->Location);

    // 2. Mock Telegram Notification
    if (_owner->TelegramEnabled) {
        printf("[TELEGRAM] Sending: ALERT! %s triggered in %s!\n", zone->Name, zone->Location);
    }

    // 3. Mock Home Assistant Update
    printf("[MQTT] Topic: sentinel/alarm/state | Payload: TRIGGERED\n");
    printf("-------------------------\n");
}

void dispatcher_send_cancel(void) {
    printf("[DISPATCHER] Sending CANCEL/RESET to all services...\n");
}

void dispatcher_update_mqtt(ArmState_t state, AlarmStatus_t status) {
    // This keeps Home Assistant in sync with the physical keypad
    const char* state_str[] = {"DISARMED", "STAY", "AWAY", "NIGHT", "VACATION"};
    printf("[MQTT] Syncing HA: Mode=%s Status=%d\n", state_str[state], status);
}