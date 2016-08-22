/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <hiredis.h>

#include "backend.h"

struct _config {
  char name[ID_MAX + 1];
  char error[256];
  bool shared;
  struct timeval timeout;
  uint8_t database;
  char    server[32];
  uint16_t port;
  redisContext *conn;
};

/* handlers */
cfg_t *
create(const char *id) {
}

bool
ready(cfg_t *cfg) {
}

char *
error(cfg_t *cfg) {
}

bool
start(cfg_t *cfg) {
}

bool
stop(cfg_t *cfg) {
}

bool
ban(cfg_t *cfg, const char *ip) {
}

bool
unban(cfg_t *cfg, const char *ip) {
}

bool
check(cfg_t *cfg, const char *ip) {
}

bool
ping(cfg_t *cfg) {
  assert(cfg != NULL);
  redisReply *reply = redisCommand(cfg->conn, "PING");
  if (reply) {
    freeReplyObject(reply);
    return true;
  }
  return false;
}

void
destroy(cfg_t *cfg) {
}
