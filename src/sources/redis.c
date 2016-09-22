/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "source.h"

#include <stdint.h>
#include <hiredis/hiredis.h>

#include "../strlcpy.h"

#define ID_MAX 32

struct _config {
  char name[ID_MAX + 1];
  char hash[ID_MAX * 2];
  char error[256];
  void (*errcb)(char *errstr);
  time_t timeout;
  uint8_t database;
  char password[32];
  char host[32];
  uint16_t port;
  redisContext *conn;
};

static bool
redis_connect(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->conn && !cfg->conn->err)
    return true; /* connected */

  redisContext *conn = NULL;
  redisReply *reply = NULL;
  do {
    struct timeval timeout = { cfg->timeout, 0 };
    conn = redisConnectWithTimeout(cfg->host, cfg->port, timeout);
    if (!conn)
      break;
    if (conn->err) {
      snprintf(cfg->error, sizeof(cfg->error), "Connection error: %s", conn->errstr);
      break;
    }
    if (cfg->password[0]) {
      reply = redisCommand(conn, "AUTH %s", cfg->password);
      if (reply->type == REDIS_REPLY_ERROR) {
        snprintf(cfg->error, sizeof(cfg->error), "auth error: %s", reply->str);
        break;
      }
      freeReplyObject(reply);
    }
    if (cfg->database) {
      reply = redisCommand(conn, "SELECT %d", cfg->database);
      if (reply->type == REDIS_REPLY_ERROR) {
        snprintf(cfg->error, sizeof(cfg->error), "auth error: %s", reply->str);
        break;
      }
      freeReplyObject(reply);
    }
    if (cfg->conn)
      redisFree(cfg->conn);
    cfg->conn = conn;
    return true;
  } while (0);

  if (conn)
    redisFree(conn);
  if (reply)
    freeReplyObject(reply);

  return false;
}

static bool
redis_disconnect(cfg_t *cfg) {
  assert(cfg != NULL);

  redisFree(cfg->conn);
  cfg->conn = NULL;
  return true;
}

cfg_t *
create(const char *init) {
  cfg_t *cfg = NULL;

  assert(init != NULL);

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;

  strlcpy(cfg->name, init, sizeof(cfg->name));
  strlcpy(cfg->hash, "f2b-banned-", sizeof(cfg->hash));
  strlcat(cfg->hash, init, sizeof(cfg->hash));

  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key   != NULL);
  assert(value != NULL);

  if (strcmp(key, "timeout") == 0) {
    cfg->timeout = atoi(value);
    return true;
  }
  if (strcmp(key, "host") == 0) {
    strlcpy(cfg->host, value, sizeof(cfg->host));
    return true;
  }
  if (strcmp(key, "port") == 0) {
    cfg->port = atoi(value);
    return true;
  }
  if (strcmp(key, "database") == 0) {
    cfg->database = atoi(value);
    return true;
  }
  if (strcmp(key, "password") == 0) {
    strlcpy(cfg->password, value, sizeof(cfg->password));
    return true;
  }

  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->host)
    return true;

  return true;
}

char *
error(cfg_t *cfg) {
  assert(cfg != NULL);

  return cfg->error;
}

void
errcb(cfg_t *cfg, void (*cb)(char *errstr)) {
  assert(cfg != NULL);
  assert(cb  != NULL);

  cfg->errcb = cb;
}

bool
start(cfg_t *cfg) {
  assert(cfg != NULL);

  return redis_connect(cfg);
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  return redis_disconnect(cfg);
}

bool
next(cfg_t *cfg, char *buf, size_t bufsize, bool reset) {
  assert(cfg != NULL);
  assert(buf != NULL);
  assert(bufsize > 0);

  assert(0); /* TODO */

  return false;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
