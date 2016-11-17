/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "source.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEFAULT_MCAST_ADDR "239.255.186.1"
#define DEFAULT_MCAST_PORT 3370

struct _config {
  char name[32];
  char error[256];
  void (*errcb)(char *errstr);
  char addr[INET_STRADDRLEN];
  char iface[16];
  uint16_t port;
  int sock;
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
  strlcpy(cfg->addr, DEFAULT_MCAST_ADDR, sizeof(cfg->addr));
  cfg->port = DEFAULT_MCAST_PORT;
  cfg->errcb = &errcb_stub;
  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key   != NULL);
  assert(value != NULL);

  if (strcmp(key, "address") == 0) {
    if (strncmp(value, "239.255.", 8) != 0) {
      strlcpy(cfg->error, "mcast address should be inside 239.255.0.0/16 block");
      return false;
    }
    strlcpy(cfg->addr, value, sizeof(cfg->addr));
    return true;
  }
  if (strcmp(key, "port") == 0) {
    cfg->port = atoi(value);
    return true;
  }
  if (strcmp(key, "iface") == 0) {
    strlcpy(cfg->iface, value, sizeof(cfg->iface));
    return true;
  }

  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->addr[0] != '\0')
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
  struct addrinfo hints;
  struct addrinfo *result;
  int opt;

  assert(cfg != NULL);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = 0;

  int ret = getaddrinfo(cfg->addr, cfg->port, &hints, &result);
  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    cfg->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (cfg->sock == -1)
      continue;
    if ((opt = fcntl(cfg->sock, F_GETFL, 0)) < 0)
      continue;
    fcntl(cfg->sock, F_SETFL, opt | O_NONBLOCK);
    opt = 1;
    setsockopt(cfg->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (connect(cfg->sock, rp->ai_addr, rp->ai_addrlen) == 0) {
      break; /* success */
  }
  freeaddrinfo(result);

  /* setsockopt -- socket(7) -- SO_BINDTODEVICE */

  if (cfg->sock < 0) {
    snprintf(cfg->error, sizeof(cfg->error), "can't bind to %s:%s", cfg->host, cfg->port);
    return false;
  }

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
  f2b_port_t *next;
  assert(cfg != NULL);

  free(cfg);
}
