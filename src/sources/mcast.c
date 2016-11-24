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
#include <net/if.h>
#include <netdb.h>

#include "../commands.h"
#include "../cmsg.h"
#include "../csocket.h"

#define DEFAULT_BIND_ADDR "0.0.0.0"
#define DEFAULT_MCAST_ADDR "239.255.186.1"
#define DEFAULT_MCAST_PORT "3370"

struct _config {
  char name[32];
  char error[256];
  void (*errcb)(const char *errstr);
  char baddr[INET6_ADDRSTRLEN]; /**< bind address */
  char maddr[INET_ADDRSTRLEN];  /**< multicast address */
  char mport[6];                /**< multicast port */
  char iface[IF_NAMESIZE];      /**< bind interface */
  int sock;
};

static void
errcb_stub(const char *str) {
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
  strlcpy(cfg->baddr, DEFAULT_BIND_ADDR,  sizeof(cfg->baddr));
  strlcpy(cfg->maddr, DEFAULT_MCAST_ADDR, sizeof(cfg->maddr));
  strlcpy(cfg->mport, DEFAULT_MCAST_PORT, sizeof(cfg->mport));
  cfg->errcb = &errcb_stub;
  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key   != NULL);
  assert(value != NULL);

  if (strcmp(key, "group") == 0) {
    if (strncmp(value, "239.255.", 8) != 0) {
      strlcpy(cfg->error, "mcast group address should be inside 239.255.0.0/16 block", sizeof(cfg->error));
      return false;
    }
    strlcpy(cfg->maddr, value, sizeof(cfg->maddr));
    return true;
  }
  if (strcmp(key, "address") == 0) {
    strlcpy(cfg->baddr, value, sizeof(cfg->baddr));
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

  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->maddr[0] && (cfg->iface[0] || cfg->baddr[0]))
    return true;

  return false;
}

char *
error(cfg_t *cfg) {
  assert(cfg != NULL);

  return cfg->error;
}

void
errcb(cfg_t *cfg, void (*cb)(const char *errstr)) {
  assert(cfg != NULL);
  assert(cb  != NULL);

  cfg->errcb = cb;
}

bool
start(cfg_t *cfg) {
  struct addrinfo hints;
  struct addrinfo *result;
  struct ip_mreq mreq;
  int opt, ret;

  assert(cfg != NULL);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = 0;

  if ((ret = getaddrinfo(cfg->baddr, cfg->mport, &hints, &result)) < 0) {
    snprintf(cfg->error, sizeof(cfg->error), "can't create socket: %s", gai_strerror(ret));
    return false;
  }

  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    /* create socket */
    if ((cfg->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0) {
      snprintf(cfg->error, sizeof(cfg->error), "can't create socket: %s", strerror(errno));
      continue;
    }
    /* set non-blocking mode */
    if ((opt = fcntl(cfg->sock, F_GETFL, 0)) < 0) {
      close(cfg->sock);
      continue;
    }
    fcntl(cfg->sock, F_SETFL, opt | O_NONBLOCK);
    /* reuse address */
    opt = 1;
    setsockopt(cfg->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    /* bind to interface if set */
    if (cfg->iface[0]) {
      if (setsockopt(cfg->sock, SOL_SOCKET, SO_BINDTODEVICE, cfg->iface, strlen(cfg->iface)) < 0) {
        snprintf(cfg->error, sizeof(cfg->error), "can't bind socket to iface %s: %s",
          cfg->iface, strerror(errno));
        close(cfg->sock);
        continue;
      }
    }
    /* bind to given address */
    if (bind(cfg->sock, rp->ai_addr, rp->ai_addrlen) < 0) {
      snprintf(cfg->error, sizeof(cfg->error), "can't bind socket to addr %s: %s",
        cfg->baddr, strerror(errno));
      close(cfg->sock);
      continue;
    }
    /* set out iface for mcast */
    if (setsockopt(cfg->sock, IPPROTO_IP, IP_MULTICAST_IF, rp->ai_addr, rp->ai_addrlen) < 0) {
      snprintf(cfg->error, sizeof(cfg->error), "can't set out iface for mcast: %s",
        strerror(errno));
      close(cfg->sock);
      continue;
    }
    /* IP_MULTICAST_LOOP -- default: yes */
    /* IP_MULTICAST_TTL  -- default: 1 */
    /* join mcast group */
    inet_pton(AF_INET, cfg->maddr, &mreq.imr_multiaddr);
    memcpy(&mreq.imr_interface, rp->ai_addr, rp->ai_addrlen);
    if (setsockopt(cfg->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
      snprintf(cfg->error, sizeof(cfg->error), "can't join mcast group: %s",
        strerror(errno));
      close(cfg->sock);
      continue;
    }
    break; /* success */
  }
  freeaddrinfo(result);

  if (cfg->sock < 0)
    return false;

  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  /* mcast group will be leaved automatically */
  close(cfg->sock);
  cfg->sock = -1;

  return true;
}

bool
next(cfg_t *cfg, char *buf, size_t bufsize, bool reset) {
  const char *args[DATA_ARGS_MAX];
  struct sockaddr_storage addr;
  socklen_t socklen;
  f2b_cmsg_t cmsg;
  int ret;

  (void)(reset);
  assert(cfg != NULL);
  assert(buf != NULL);
  assert(bufsize > 0);

  memset(&cmsg, 0x0, sizeof(cmsg));
  memset(&addr, 0x0, sizeof(addr));

  while (1) {
    ret = f2b_csocket_recv(cfg->sock, &cmsg, &addr, &socklen);
    if (ret == 0)
      break; /* no messages */
    if (ret < 0) {
      cfg->errcb(f2b_csocket_error(ret));
      continue;
    }
    /* ret > 0 */
    if (cmsg.type != CMD_JAIL_IP_BAN)
      continue;
    ret = f2b_cmsg_extract_args(&cmsg, args);
    if (!f2b_cmd_check_argc(cmsg.type, ret)) {
      cfg->errcb("received cmsg with wrong args count");
      continue;
    }
    if (strcmp(cfg->name, args[0]) != 0)
      continue; /* wrong group */
    strlcpy(buf, args[1], bufsize);
    return true;
  }

  return false;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
