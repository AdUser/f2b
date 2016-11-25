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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>

#include "../strlcpy.h"
#include "../commands.h"
#include "../cmsg.h"
#include "../csocket.h"

#include "backend.h"
#include "shared.c"

#define DEFAULT_MCAST_ADDR "239.255.186.1"
#define DEFAULT_MCAST_PORT "3370"
#define DEFAULT_PING_NUM 5

struct _config {
  char name[ID_MAX + 1];
  char error[256];
  bool shared;
  uint8_t ping_num; /*< current number of ping() call */
  uint8_t ping_max; /*< max ping() calls before actually send CMD_PING packet */
  char maddr[INET_ADDRSTRLEN];  /**< multicast address */
  char mport[6];                /**< multicast port */
  char iface[IF_NAMESIZE];      /**< bind interface */
  int sock;
  struct sockaddr_storage sa;
  socklen_t sa_len;
};

cfg_t *
create(const char *id) {
  cfg_t *cfg = NULL;

  assert(id != NULL);

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  cfg->ping_max = DEFAULT_PING_NUM;
  strlcpy(cfg->name, id, sizeof(cfg->name));
  strlcpy(cfg->maddr, DEFAULT_MCAST_ADDR, sizeof(cfg->maddr));
  strlcpy(cfg->mport, DEFAULT_MCAST_PORT, sizeof(cfg->mport));

  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key != NULL);
  assert(value != NULL);

  if (strcmp(key, "group") == 0) {
    if (strncmp(value, "239.255.", 8) != 0) {
      strlcpy(cfg->error, "mcast group address should be inside 239.255.0.0/16 block", sizeof(cfg->error));
      return false;
    }
    strlcpy(cfg->maddr, value, sizeof(cfg->maddr));
    return true;
  }
  if (strcmp(key, "port") == 0) {
    strlcpy(cfg->mport, value, sizeof(cfg->mport));
    return true;
  }
  if (strcmp(key, "iface") == 0) {
    strlcpy(cfg->iface, value, sizeof(cfg->iface));
    return true;
  }
  if (strcmp(key, "ping") == 0) {
    cfg->ping_max = atoi(value);
    return true;
  }

  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->maddr[0] && cfg->mport[0])
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
  struct addrinfo hints;
  struct addrinfo *result;
  assert(cfg != NULL);
  int ret, sock = -1;

  if (cfg->shared && usage_inc(cfg->name) > 1)
    return true;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = 0;

  if ((ret = getaddrinfo(cfg->maddr, cfg->mport, &hints, &result)) < 0) {
    snprintf(cfg->error, sizeof(cfg->error), "can't create socket: %s", gai_strerror(ret));
    return false;
  }

  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    if (sock >= 0) {
      close(sock); /* from prev iteration */
      sock = -1;
    }
    if ((sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0) {
      snprintf(cfg->error, sizeof(cfg->error), "can't create socket: %s", strerror(errno));
      continue;
    }
    memcpy(&cfg->sa, rp->ai_addr, rp->ai_addrlen);
    cfg->sa_len = rp->ai_addrlen;
    break;
  }
  freeaddrinfo(result);

  if (sock < 0)
    return false;

  cfg->sock = sock;
  return true;

  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->shared && usage_dec(cfg->name) > 0)
    return true;

  close(cfg->sock);
  cfg->sock = -1;

  return true;
}

bool
ban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  /* TODO */

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

  (void)(ip); /* suppress warning for unused variable 'ip' */
  return false;
}

bool
ping(cfg_t *cfg) {
  assert(cfg != NULL);

  cfg->ping_num++;
  if (cfg->ping_num < cfg->ping_max)
    return true; /* skip this try */

  /* max empty calls reached, make real ping */
  cfg->ping_num = 0;
  /* TODO */

  return false;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
