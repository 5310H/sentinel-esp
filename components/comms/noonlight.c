#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "noonlight.h"

/* State: Array to hold active Alarm IDs for each service type */
static char* active_alarm_ids[4] = { NULL, NULL, NULL, NULL };

/**
 * INTERNAL: Helper to map zone type to internal slot.
 */
static int _get_slot(const char *type) {
    if (!type) return -1;
    if (strcmp(type, "police") == 0)  return SLOT_POLICE;
    if (strcmp(type, "fire") == 0)    return SLOT_FIRE;
    if (strcmp(type, "medical") == 0) return SLOT_MEDICAL;
    if (strcmp(type, "other") == 0)   return SLOT_OTHER;
    return -1;
}

/**
 * Returns active ID for testing/verification.
 */
char* noonlight_get_active_id(int slot) {
    if (slot < 0 || slot > 3) return NULL;
    return active_alarm_ids[slot];
}

/**
 * 1. SYNC PEOPLE (POST /v1/alarms/{id}/people)
 */
void noonlight_sync_people(struct owner *o, const char *alarm_id, struct contact *c, int c_count) {
    if (!alarm_id || !c) return;
    for (int i = 0; i < c_count; i++) {
        if (c[i].notify && strcmp(c[i].notify, "service") == 0) {
            char *json = NULL;
            asprintf(&json, "{\"name\":\"%s\",\"phone\":\"%s\",\"pin\":\"%s\"}",
                     c[i].name, c[i].phone, c[i].pin);

            printf("\n[SYNC CONTACT] URL: %s/%s/people\n", o->MonitorServiceID, alarm_id);
            printf("[AUTH] Bearer %s\n", o->MonitorServiceKey);
            printf("[JSON] %s\n", json);
            free(json);
        }
    }
}

/**
 * 2. SEND EVENT (POST /v1/alarms/{id}/events)
 */
void noonlight_send_event(struct owner *o, const char *alarm_id, struct zone *z) {
    char *json = NULL;
    asprintf(&json, "{\"event_type\":\"%s\",\"event_source\":\"%s\",\"event_body\":{\"value\":\"%s\",\"mfg\":\"%s\",\"model\":\"%s\"}}",
             z->type, z->location, z->description, z->mfg, z->model);

    printf("\n[ADD EVENT] URL: %s/%s/events\n", o->MonitorServiceID, alarm_id);
    printf("[AUTH] Bearer %s\n", o->MonitorServiceKey);
    printf("[JSON] %s\n", json);
    free(json);
}

/**
 * 3. CANCEL ALARM (POST /v1/alarms/{id}/status)
 */
void noonlight_cancel_alarm(struct owner *o, int slot, const char *entered_pin) {
    if (slot < 0 || slot > 3 || active_alarm_ids[slot] == NULL) return;

    char *json = NULL;
    asprintf(&json, "{\"status\":\"CANCELED\",\"pin\":\"%s\"}", entered_pin);

    printf("\n[CANCEL] URL: %s/%s/status\n", o->MonitorServiceID, active_alarm_ids[slot]);
    printf("[AUTH] Bearer %s\n", o->MonitorServiceKey);
    printf("[JSON] %s\n", json);

    free(active_alarm_ids[slot]);
    active_alarm_ids[slot] = NULL;
    free(json);
}

/**
 * 4. HANDLE TRIGGER (Main Entry Point)
 */
void noonlight_handle_trigger(struct owner *o, struct zone *z, struct contact *c, int c_count) {
    int slot = _get_slot(z->type);
    if (slot == -1) return;

    /* Gatekeeper Check */
    bool enabled = false;
    if (slot == SLOT_POLICE)       enabled = o->monitor_police;
    else if (slot == SLOT_FIRE)    enabled = o->monitor_fire;
    else if (slot == SLOT_MEDICAL) enabled = o->monitor_medical;
    else if (slot == SLOT_OTHER)   enabled = o->monitor_other;

    if (!enabled) return;

    if (active_alarm_ids[slot] == NULL) {
        /* INITIAL ALARM CREATION */
        char *json = NULL;
        asprintf(&json, "{\"name\":\"%s\",\"phone\":\"%s\",\"location\":{\"address\":{\"line1\":\"%s\",\"zip\":\"%s\"}},\"services\":{\"%s\":true},\"instructions\":{\"entry\":\"%s: %s\"}}",
                 o->name, o->phone, o->address1, o->zip, z->type, z->location, z->description);

        printf("\n[NEW ALARM] URL: %s\n", o->MonitorServiceID);
        printf("[AUTH] Bearer %s\n", o->MonitorServiceKey);
        printf("[JSON] %s\n", json);

        /* Simulation: Capturing mock ID */
        char mock[32];
        snprintf(mock, sizeof(mock), "ID_%s_XYZ", z->type);
        active_alarm_ids[slot] = strdup(mock);

        /* Automatic Contact Sync */
        noonlight_sync_people(o, active_alarm_ids[slot], c, c_count);
        free(json);
    } else {
        /* EXISTING INCIDENT - SEND EVENT */
        noonlight_send_event(o, active_alarm_ids[slot], z);
    }
}