/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "source.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define HOST_MAX 48
#define PORT_MAX 6

typedef struct f2b_port_t {
  struct f2b_port_t *next;
  char host[HOST_MAX];
  char port[PORT_MAX];
  int fd;
} f2b_port_t;

struct _config {
  char name[32];
  char error[256];
  void (*errcb)(char *errstr);
  f2b_port_t *ports;
  f2b_port_t *current;
};

static void
errcb_stub(char *str) {
  assert(str != NULL);
  (void)(str);
}

cfg_t *
create(const char *init) {
  cfg_t *cfg = NULL;
  assert(init != NULL);
  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  strlcpy(cfg->name, init, sizeof(cfg->name));
  cfg->errcb = &errcb_stub;
  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key   != NULL);
  assert(value != NULL);

  if (strcmp(key, "listen") == 0) {
    void *buf = NULL;
    if (strchr(value, ':') == NULL) {
      cfg->listen_af = AF_INET;
      buf = &cfg->listen_addr.v4;
    } else {
      cfg->listen_af = AF_INET6;
      buf = &cfg->listen_addr.v6;
    }
    if (inet_pton(cfg->listen_af, value, buf) <= 0) {
      snprintf(cfg->error, sizeof(cfg->error), "invalid listen address: %s", value);
      return false;
    }
    return true;
  }
  if (strcmp(key, "port") == 0) {
    if (cfg->ports_used >= MAX_PORTS) {
      strlcpy(cfg->error, "max ports number reached in this portknock instance", sizeof(cfg->error));
      return false;
    }
    cfg->ports[cfg->ports_used] = atoi(value);
    if (cfg->ports[cfg->ports_used] == 0) {
      snprintf(cfg->error, sizeof(cfg->error), "invalid port number: %s", value);
      return false;
    }
    cfg->ports_used++;
  }

  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);
  if (cfg->ports)
    return true;
  return false;
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

  /* TODO */
  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  /* TODO */
  return true;
}

bool
next(cfg_t *cfg, char *buf, size_t bufsize, bool reset) {
  assert(cfg != NULL);
  assert(buf != NULL);
  assert(bufsize > 0);

  /* TODO */
  return false;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
