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

static void
errcb_stub(char *str) {
  assert(str != NULL);
  (void)(str);
}

static bool
redis_connect(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->conn && !cfg->conn->err)
    return true; /* connected */

  redisContext *conn = NULL;
  redisReply *reply = NULL;
  do {
    struct timeval timeout = { .tv_sec = cfg->timeout, .tv_usec = 0 };
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
    timeout.tv_sec  = 0;
    timeout.tv_usec = 10000; /* 0.01s */
    if (redisSetTimeout(conn, timeout) != REDIS_OK) {
      strlcpy(cfg->error, "can't enable nonblocking mode", sizeof(cfg->error));
      break;
    }
    reply = redisCommand(conn, "SUBSCRIBE %s", cfg->hash);
    if (reply->type == REDIS_REPLY_ERROR) {
      snprintf(cfg->error, sizeof(cfg->error), "can't subscribe: %s", reply->str);
      break;
    }
    freeReplyObject(reply);
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
  cfg->errcb = &errcb_stub;

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
  bool gotit = false;
  assert(cfg != NULL);
  assert(buf != NULL);
  assert(bufsize > 0);

  (void)(reset); /* suppress warning */

  if (!cfg->conn || cfg->conn->err)
    redis_connect(cfg);
  if (!cfg->conn)
    return false; /* reconnect failure */

  if (cfg->conn->err) {
    snprintf(cfg->error, sizeof(cfg->error), "connection error: %s", cfg->conn->errstr);
    return false;
  }

  redisReply *reply = NULL;
  if (redisGetReply(cfg->conn, (void **) &reply) == REDIS_OK) {
    if (reply->type == REDIS_REPLY_ARRAY) {
      if (strcmp(reply->element[0]->str, "message") == 0 ||
          strcmp(reply->element[1]->str, cfg->hash) == 0) {
        strlcpy(buf, reply->element[2]->str, bufsize);
        gotit = true;
      } else {
        cfg->errcb(cfg->error);
      }
    } else {
      strlcpy(cfg->error, "reply is not a array type",  sizeof(cfg->error));
      cfg->errcb(cfg->error);
    }
    freeReplyObject(reply);
  } else if (cfg->conn->err == REDIS_ERR_IO && errno == EAGAIN) {
    cfg->conn->err = 0; /* reset error to prevent reconnecting */
  } else {
    snprintf(cfg->error, sizeof(cfg->error),  "can't get reply from server %s: %s", cfg->host, cfg->conn->errstr);
    cfg->errcb(cfg->error);
  }

  return gotit;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
