#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "hal.h"
#include "notifier.h"
#include "config.h"

// --- Private Global State ---
static ArmState_t    _arm_mode = ARMSTATE_DISARMED;
static AlarmStatus_t _status   = ALARM_IDLE;
static int           _timer    = 0;

// --- Private Data Pointers ---
static Owner_t* _owner = NULL;
static User_t* _users = NULL;
static int      _u_count = 0;
static Zone_t* _zones = NULL;
static int      _z_count = 0;
static Relay_t* _relays = NULL;
static int      _r_count = 0;
static Rule_t* _rules = NULL;
static int      _rule_count = 0;

static int get_relay_gpio(int relay_id) {
    for (int i = 0; i < _r_count; i++) {
        if (_relays[i].ID == relay_id) return _relays[i].PinNumber;
    }
    return -1;
}

void engine_init(Owner_t *o, User_t *u, int uc, Zone_t *z, int zc, Relay_t *r, int rc, Rule_t *ru, int ruc) {
    _owner = o; _users = u; _u_count = uc;
    _zones = z; _z_count = zc;
    _relays = r; _r_count = rc;
    _rules = ru; _rule_count = ruc;
    notifier_init(_owner->MonitoringURL, _owner->AccountID);
}

void engine_arm(ArmState_t mode) {
    _arm_mode = mode;
    _status = ALARM_EXITING;
    _timer = _owner->ExitDelay;
    notifier_send(NOTIFY_ARM_STATE, "System Arming Sequence Started");
}

void engine_disarm(const char* input_pin) {
    for (int i = 0; i < _u_count; i++) {
        if (strcmp(_users[i].PIN, input_pin) == 0) {
            _arm_mode = ARMSTATE_DISARMED;
            _status = ALARM_IDLE;
            _timer = 0;
            
            // Hardware Silence
            hal_set_relay(get_relay_gpio(1), false); // Siren
            
            // Noonlight Cancellation
            notifier_cancel_alarm();
            
            char msg[64];
            snprintf(msg, sizeof(msg), "DISARMED by %s", _users[i].Name);
            notifier_send(NOTIFY_USER_ACTION, msg);
            return;
        }
    }
    notifier_send(NOTIFY_ALARM, "Unauthorized Access: Invalid PIN");
}

void engine_process_rules(int zone_id) {
    // 1. Rules/Automation
    for (int i = 0; i < _rule_count; i++) {
        if (_rules[i].Enabled && _rules[i].TriggerZoneID == zone_id) {
            int gpio = get_relay_gpio(_rules[i].ActionRelayID);
            if (gpio != -1) hal_set_relay(gpio, true);
        }
    }

    // 2. Security Logic
    for (int i = 0; i < _z_count; i++) {
        if (_zones[i].ID == zone_id) {
            // Panic Logic (Instant)
            if (strstr(_zones[i].Name, "Panic") != NULL) {
                _status = ALARM_TRIGGERED;
                hal_set_relay(get_relay_gpio(1), true);
                notifier_send(NOTIFY_ALARM, "PANIC BUTTON PRESSED");
                return;
            }
            // Armed Intrusion
            if (_arm_mode != ARMSTATE_DISARMED && _status == ALARM_IDLE) {
                if (_zones[i].IsPerimeter) {
                    _status = ALARM_PENDING;
                    _timer = _owner->EntryDelay;
                } else {
                    _status = ALARM_TRIGGERED;
                    hal_set_relay(get_relay_gpio(1), true);
                    notifier_send(NOTIFY_ALARM, "INSTANT ALARM: Interior Intrusion");
                }
            }
        }
    }
}

void engine_tick() {
    if (_timer > 0) {
        _timer--;
        if (_status == ALARM_PENDING && _timer % 2 == 0) hal_buzzer_beep(100);
        if (_timer == 0) {
            if (_status == ALARM_EXITING) _status = ALARM_IDLE;
            else if (_status == ALARM_PENDING) {
                _status = ALARM_TRIGGERED;
                hal_set_relay(get_relay_gpio(1), true);
                notifier_send(NOTIFY_ALARM, "ALARM TRIGGERED: Entry Timeout");
            }
        }
    }
}

ArmState_t engine_get_arm_state() { return _arm_mode; }
AlarmStatus_t engine_get_status() { return _status; }