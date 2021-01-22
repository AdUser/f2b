/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdint.h>
#include <hiredis/hiredis.h>

#include "../strlcpy.h"

#include "source.h"

#define MODNAME "redis"
#define ID_MAX 32

struct _config {
  redisContext *conn;
  void (*logcb)(enum loglevel lvl, const char *msg);
  time_t timeout;
  int flags;
  uint16_t port;
  uint32_t received;
  uint8_t database;
  char password[32];
  char host[32];
  char name[ID_MAX + 1];
  char hash[ID_MAX * 2];
};

#include "source.c"

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
      log_msg(cfg, error, "connection error: %s", conn->errstr);
      break;
    }
    if (cfg->password[0]) {
      if ((reply = redisCommand(conn, "AUTH %s", cfg->password)) == NULL)
        break;
      if (reply->type == REDIS_REPLY_ERROR) {
        log_msg(cfg, error, "auth error: %s", reply->str);
        break;
      }
      freeReplyObject(reply);
    }
    if (cfg->database) {
      if ((reply = redisCommand(conn, "SELECT %d", cfg->database)) == NULL)
        break;
      if (reply->type == REDIS_REPLY_ERROR) {
        log_msg(cfg, error, "reply error: %s", reply->str);
        break;
      }
      freeReplyObject(reply);
    }
    timeout.tv_sec  = 0;
    timeout.tv_usec = 10000; /* 0.01s */
    if (redisSetTimeout(conn, timeout) != REDIS_OK) {
      log_msg(cfg, error, "can't enable nonblocking mode");
      break;
    }
    if ((reply = redisCommand(conn, "SUBSCRIBE %s", cfg->hash)) == NULL)
      break;
    if (reply->type == REDIS_REPLY_ERROR) {
      log_msg(cfg, error, "can't subscribe: %s", reply->str);
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

  if (cfg->conn) {
    redisFree(cfg->conn);
    cfg->conn = NULL;
  }
  return true;
}

cfg_t *
create(const char *init) {
  cfg_t *cfg = NULL;

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;

  strlcpy(cfg->hash, "f2b-banned-", sizeof(cfg->hash));
  strlcat(cfg->hash, init, sizeof(cfg->hash));
  cfg->logcb = &logcb_stub;
  cfg->flags |= MOD_TYPE_SOURCE;
  if (init && strlen(init) > 0) {
    strlcpy(cfg->name, init, sizeof(cfg->name));
    cfg->flags |= MOD_IS_READY;
  }
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
start(cfg_t *cfg) {
  assert(cfg != NULL);

  redis_connect(cfg); /* may fail */
  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  redis_disconnect(cfg);
  return true;
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
    log_msg(cfg, error, "connection error: %s", cfg->conn->errstr);
    return false;
  }

  redisReply *reply = NULL;
  if (redisGetReply(cfg->conn, (void **) &reply) == REDIS_OK) {
    cfg->received++;
    if (reply->type == REDIS_REPLY_ARRAY) {
      if (strcmp(reply->element[0]->str, "message") == 0 ||
          strcmp(reply->element[1]->str, cfg->hash) == 0) {
        strlcpy(buf, reply->element[2]->str, bufsize);
        gotit = true;
      } else {
        log_msg(cfg, error, "wrong redis message type: %s", reply->element[0]->str);
      }
    } else {
      log_msg(cfg, error, "reply is not a array type");
    }
    freeReplyObject(reply);
  } else if (cfg->conn->err == REDIS_ERR_IO && errno == EAGAIN) {
    cfg->conn->err = 0; /* reset error to prevent reconnecting */
  } else {
    log_msg(cfg, error, "can't get reply from server %s: %s", cfg->host, cfg->conn->errstr);
  }

  return gotit;
}

bool
stats(cfg_t *cfg, char *buf, size_t bufsize) {
  const char *fmt =
    "connected: %s\n"
    "last error: %d (%s)\n"
    "messages: %u\n";

  assert(cfg != NULL);

  if (buf == NULL || bufsize == 0)
    return false;

  if (cfg->conn) {
    const char *err = cfg->conn->errstr[0] == '\0' ? cfg->conn->errstr : "---";
    snprintf(buf, bufsize, fmt, "yes", cfg->conn->err, err, cfg->received);
  } else {
    snprintf(buf, bufsize, fmt, "no", "0", "---", cfg->received);
  }

  return true;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
