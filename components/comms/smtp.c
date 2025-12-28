#include "hardware_config.h"

void smtp_send_alert(struct owner *o, struct zone *z, struct contact *c) {
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
        o->SmtpUser, c->phone, z->location,
        z->description, z->location, z->type);

    printf("\n[SMTP ENGINE - W5500 ETHERNET READY]\n");
    printf("Target: %s:%d\n", o->SmtpServer, o->SmtpPort);
    printf("Data:\n%s\n", data_packet);

    free(data_packet);
}