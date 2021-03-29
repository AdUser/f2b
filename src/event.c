/* Copyright 2021 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"

static void (*handlers[4])(const char *) = { NULL, NULL, NULL, NULL };

void
f2b_event_send(const char *evt) {
  for (short int i = 0; i < 3; i++) {
    if (!handlers[i]) break; /* end of list */
    handlers[i](evt);
  }
}

bool
f2b_event_handler_register(void (*h)(const char *)) {
  for (short int i = 0; i < 3; i++) {
    if (handlers[i] == NULL) {
      handlers[i] = h;
      return true;
    }
  }
  return false;
}

void
f2b_event_handler_flush() {
  memset(handlers, 0x0, sizeof(handlers));
}
