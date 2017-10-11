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

#include "../strlcpy.h"

#include "backend.h"
#include "shared.c"

struct _config {
  char name[ID_MAX + 1];
  char hash[ID_MAX * 2];
  char error[256];
  bool shared;
  time_t timeout;
  uint8_t ping_num; /*< current number of ping() call */
  uint8_t ping_max; /*< max ping() calls before actually pinginig redis server */
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
      if ((reply = redisCommand(conn, "AUTH %s", cfg->password)) == NULL)
        break;
      if (reply->type == REDIS_REPLY_ERROR) {
        snprintf(cfg->error, sizeof(cfg->error), "auth error: %s", reply->str);
        break;
      }
      freeReplyObject(reply);
    }
    if (cfg->database) {
      if ((reply = redisCommand(conn, "SELECT %d", cfg->database)) == NULL)
        break;
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

  if (cfg->conn) {
    redisFree(cfg->conn);
    cfg->conn = NULL;
  }
  return true;
}

cfg_t *
create(const char *id) {
  cfg_t *cfg = NULL;

  assert(id != NULL);

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  strlcpy(cfg->name, id, sizeof(cfg->name));
  strlcpy(cfg->hash, "f2b-banned-", sizeof(cfg->hash));
  strlcat(cfg->hash, id, sizeof(cfg->hash));

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
    strlcpy(cfg->host, value, sizeof(cfg->host));
    return true;
  }
  if (strcmp(key, "port") == 0) {
    cfg->port = atoi(value);
    return true;
  }
  if (strcmp(key, "ping") == 0) {
    cfg->ping_max = atoi(value);
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

  if (cfg->shared)
    usage_inc(cfg->name);

  redis_connect(cfg); /* may fail */
  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->shared && usage_dec(cfg->name) > 0)
    return true; /* skip disconnect, if not last user */

  redis_disconnect(cfg);
  return true;
}

bool
ban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  if (!redis_connect(cfg))
    return false;

  redisReply *reply;
  do {
    if ((reply = redisCommand(cfg->conn, "HINCRBY %s %s %d", cfg->hash, ip, 1)) == NULL)
      break;
    if (reply->type == REDIS_REPLY_ERROR) {
      snprintf(cfg->error, sizeof(cfg->error), "HINCRBY: %s", reply->str);
      break;
    }
    freeReplyObject(reply);
    if ((reply = redisCommand(cfg->conn, "PUBLISH %s %s",    cfg->hash, ip)) == NULL)
      break;
    if (reply->type == REDIS_REPLY_ERROR) {
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

  (void)(cfg); /* suppress warning for unused variable 'ip' */
  (void)(ip);  /* suppress warning for unused variable 'ip' */

  return true;
}

bool
check(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  (void)(cfg); /* suppress warning for unused variable 'ip' */
  (void)(ip);  /* suppress warning for unused variable 'ip' */

  return false;
}

bool
ping(cfg_t *cfg) {
  assert(cfg != NULL);

  if (!cfg->conn || cfg->conn->err)
    redis_connect(cfg);
  if (!cfg->conn)
    return false; /* reconnect failure */

  if (cfg->conn->err) {
    snprintf(cfg->error, sizeof(cfg->error), "connection error: %s", cfg->conn->errstr);
    return false;
  }

  cfg->ping_num++;
  if (cfg->ping_num < cfg->ping_max)
    return true; /* skip this try */

  /* max empty calls reached, make real ping */
  cfg->ping_num = 0;
  redisReply *reply = redisCommand(cfg->conn, "PING");
  if (reply) {
    bool result = true;
    if (reply->type == REDIS_REPLY_ERROR) {
      strlcpy(cfg->error, reply->str, sizeof(cfg->error));
      result = false;
    }
    freeReplyObject(reply);
    return result;
  }

  return false;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
