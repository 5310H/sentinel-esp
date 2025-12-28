#include <stdio.h>
#include <string.h>
#include "dispatch.h"

char payload_buffer[1024];

void payload_build_noonlight(struct owner *o, struct zone *z, bool is_update) {
    if (!is_update) {
        snprintf(payload_buffer, 1024, "{\"service_id\":\"%s\",\"location\":{\"address\":{\"line1\":\"%s\"}}}",
                 o->MonitorServiceID, o->address1);
    } else {
        snprintf(payload_buffer, 1024, "{\"event_type\":\"%s\",\"event_body\":{\"value\":\"%s\"}}",
                 z->type, z->description);
    }
}

void payload_build_telegram(struct owner *o, struct zone *z) {
    snprintf(payload_buffer, 1024, "{\"chat_id\":\"%s\",\"text\":\"Alarm: %s\"}",
             o->telegramChatID, z->location);
}

void payload_build_smtp(struct owner *o, struct zone *z, struct contact *c) {
    (void)o; // Silence warning
    snprintf(payload_buffer, 1024, "To: %s\r\nSubject: Alert\r\n\r\n%s", c->phone, z->description);
}

bool payload_parse_response(int slot, const char *raw, char *out_id) {
    if (slot == SLOT_NOONLIGHT) {
        char *p = strstr(raw, "\"id\":\"");
        if (p) {
            sscanf(p + 6, "%[^\"]", out_id);
            return true;
        }
    } else if (slot == SLOT_TELEGRAM) {
        return strstr(raw, "\"ok\":true") != NULL;
    } else if (slot == SLOT_SMTP) {
        return strstr(raw, "250") != NULL;
    }
    return false;
}