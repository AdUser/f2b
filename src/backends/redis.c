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
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <hiredis/hiredis.h>

#include "backend.h"
#include "shared.c"

struct _config {
  char name[ID_MAX + 1];
  char hash[ID_MAX * 2];
  char error[256];
  bool shared;
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

  redisReply *reply;
  struct timeval timeout = { cfg->timeout, 0 };
  do {
    cfg->conn = redisConnectWithTimeout(cfg->host, cfg->port, timeout);
    if (cfg->conn->err) {
      snprintf(cfg->error, sizeof(cfg->error), "Connection error: %s", cfg->conn->errstr);
      return false;
    }
    if (cfg->password[0]) {
      reply = redisCommand(cfg->conn, "AUTH %s", cfg->password);
      if (reply->type == REDIS_REPLY_ERROR) {
        snprintf(cfg->error, sizeof(cfg->error), "auth error: %s", reply->str);
        freeReplyObject(reply);
        break;
      }
      freeReplyObject(reply);
    }
    if (cfg->database) {
      reply = redisCommand(cfg->conn, "SELECT %d", cfg->database);
      if (reply->type == REDIS_REPLY_ERROR) {
        snprintf(cfg->error, sizeof(cfg->error), "auth error: %s", reply->str);
        freeReplyObject(reply);
        break;
      }
      freeReplyObject(reply);
    }
    return true;
  } while (0);

  redisFree(cfg->conn);
  cfg->conn = NULL;
  return false;
}

static bool
redis_disconnect(cfg_t *cfg) {
  assert(cfg != NULL);

  redisFree(cfg->conn);
  cfg->conn = NULL;
  return true;
}

static bool
redis_reconnect(cfg_t *cfg) {
  redis_disconnect(cfg);
  return redis_connect(cfg);
}

/* handlers */
cfg_t *
create(const char *id) {
  cfg_t *cfg = NULL;

  assert(id != NULL);

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  snprintf(cfg->name, sizeof(cfg->name), "%s", id);
  snprintf(cfg->hash, sizeof(cfg->hash), "f2b-banned-%s", id);

  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key != NULL);
  assert(value != NULL);

  if (strcmp(key, "timeout") == 0) {
    cfg->timeout = atoi(value);
    return true;
  }
  if (strcmp(key, "shared") == 0) {
    cfg->shared = (strcmp(value, "yes") ? true : false);
    return true;
  }
  if (strcmp(key, "host") == 0) {
    snprintf(cfg->host, sizeof(cfg->host), "%s", value);
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
    snprintf(cfg->password, sizeof(cfg->password), "%s", value);
    return true;
  }

  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->host)
    return true;

  return false;
}

char *
error(cfg_t *cfg) {
  assert(cfg != NULL);

  return cfg->error;
}

bool
start(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->shared && usage_inc(cfg->name) > 1)
    return true;

  redis_connect(cfg);
  return false;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->shared && usage_dec(cfg->name) > 0)
    return true;

  redis_disconnect(cfg);
  return true;
}

bool
ban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  redisReply *reply;
  do {
    reply = redisCommand(cfg->conn, "HINCRBY %s %s %d", cfg->hash, ip, 1);
    if (reply && reply->type == REDIS_REPLY_ERROR) {
      snprintf(cfg->error, sizeof(cfg->error), "HINCRBY: %s", reply->str);
      break;
    }
    freeReplyObject(reply);
    reply = redisCommand(cfg->conn, "PUBLISH %s %s",    cfg->hash, ip);
    if (reply && reply->type == REDIS_REPLY_ERROR) {
      snprintf(cfg->error, sizeof(cfg->error), "PUBLISH: %s", reply->str);
      break;
    }
    freeReplyObject(reply);
    return true;
  } while (0);

  if (reply)
    freeReplyObject(reply);

  return false;
}

bool
unban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);
  (void)(ip); /* suppress warning for unused variable 'ip' */
  return true;
}

bool
check(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  bool exists;
  redisReply *reply;
  do {
    reply = redisCommand(cfg->conn, "HEXISTS %s %s", cfg->hash, ip);
    if (reply && reply->type == REDIS_REPLY_ERROR) {
      snprintf(cfg->error, sizeof(cfg->error), "PUBLISH: %s", reply->str);
      break;
    }
    if (reply && reply->integer == REDIS_REPLY_INTEGER)
      exists = reply->integer ? true : false;
    freeReplyObject(reply);
    return exists;
  } while (0);

  if (reply)
    freeReplyObject(reply);

  return false;
}

bool
ping(cfg_t *cfg) {
  assert(cfg != NULL);
  redisReply *reply = redisCommand(cfg->conn, "PING");
  if (cfg->conn->err) {
    snprintf(cfg->error, sizeof(cfg->error), "connection error: %s", cfg->conn->errstr);
    redis_reconnect(cfg);
    return cfg->conn->err ? false : true;
  }
  if (reply && reply->type == REDIS_REPLY_ERROR) {
    snprintf(cfg->error, sizeof(cfg->error), "%s", reply->str);
    freeReplyObject(reply);
    return false;
  } else {
    return false;
  }
  return true;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
