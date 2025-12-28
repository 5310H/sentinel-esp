#include "dispatcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "hardware_config.h"

void smtp_send_alert(owner_t *o, zone_t *z, user_t *c) {
    char *data_packet = NULL;
    
    // We add specific KC868 context to the email so the user knows which board triggered
    asprintf(&data_packet,
        "From: KC868-Sentinel <%s>\r\n"
        "To: %s\r\n"
        "Subject: [ALARM] %s\r\n"
        "\r\n"
        "Hardware ID: KinCony KC868 (ESP32)\r\n"
        "Trigger: %s at %s\r\n"
        "Sensor Type: %s\r\n"
        "Hardware Port: PCF8575_IO\r\n",
        o->smtp_user, c->phone, z->location,
        z->description, z->location, z->type);

    printf("\n[SMTP ENGINE - W5500 ETHERNET READY]\n");
    printf("Target: %s:%d\n", o->smtp_server, o->smtp_port);
    printf("Data:\n%s\n", data_packet);

    free(data_packet);
}