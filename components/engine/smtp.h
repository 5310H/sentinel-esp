#ifndef SMTP_H
#define SMTP_H

#include "storage_mgr.h"

void smtp_send_email(owner_t *owner, const char *subject, const char *body);

#endif