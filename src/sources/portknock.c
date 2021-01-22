/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <ctype.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "source.h"
#define MODNAME "portknock"
#define HOST_MAX 48
#define PORT_MAX 6

typedef struct f2b_port_t {
  struct f2b_port_t *next;
  unsigned int accepts;
  uint32_t stag;
  int sock;
  char host[HOST_MAX];
  char port[PORT_MAX];
} f2b_port_t;

struct _config {
  void (*logcb)(enum loglevel lvl, const char *msg);
  f2b_port_t *ports;
  f2b_port_t *current;
  int flags;
  char name[32];
};

#include "source.c"

static bool
try_parse_listen_opt(f2b_port_t *port, const char *value) {
  char buf[256];
  char *p;

  strlcpy(buf, value, sizeof(buf));
  if (*buf == '[') {
    /* IPv6, expected: [XXXX::XXXX:XXXX]:YYYY */
    if ((p = strstr(buf, "]:")) == NULL) {
      *p = '\0', p += 2;
      strlcpy(port->port, p, sizeof(port->port));
      p = buf + 1;
      strlcpy(port->host, p, sizeof(port->host));
      return true;
    }
    return false; /* can't find port */
  }
  if ((p = strchr(buf, ':')) != NULL) {
    /* IPv4, expected: XX.XX.XX.XX:YYYY */
    *p = '\0', p += 1;
    strlcpy(port->port, p, sizeof(port->port));
    p = buf;
    strlcpy(port->host, p, sizeof(port->host));
    return true;
  }
  if (isdigit(*buf) && strlen(buf) <= 5) {
    /* IPv4, expected: YYYY */
    strlcpy(port->host, "0.0.0.0", sizeof(port->host));
    strlcpy(port->port, buf,       sizeof(port->port));
    return true;
  }

  return false;
}

cfg_t *
create(const char *init) {
  cfg_t *cfg = NULL;
  assert(init != NULL);
  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  strlcpy(cfg->name, init, sizeof(cfg->name));
  cfg->logcb = &logcb_stub;
  cfg->flags |= MOD_TYPE_SOURCE;
  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key   != NULL);
  assert(value != NULL);

  if (strcmp(key, "listen") == 0) {
    f2b_port_t *port = NULL;
    if ((port = calloc(1, sizeof(f2b_port_t))) == NULL) {
      log_msg(cfg, error, "out of memory");
      return false;
    }
    if (try_parse_listen_opt(port, value) == false) {
      log_msg(cfg, error, "can't parse: %s", value);
      free(port);
      return false;
    }
    port->stag = fnv_32a_str(port->port, FNV1_32A_INIT);
    port->next = cfg->ports;
    cfg->ports = port;
    cfg->flags |= MOD_IS_READY;
    return true;
  }

  return false;
}

bool
start(cfg_t *cfg) {
  struct addrinfo hints;
  struct addrinfo *result;
  int opt;

  assert(cfg != NULL);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  for (f2b_port_t *port = cfg->ports; port != NULL; port = port->next) {
    port->sock = -1;
    int ret = getaddrinfo(port->host, port->port, &hints, &result);
    if (ret != 0) {
      log_msg(cfg, error, "getaddrinfo: %s", gai_strerror(ret));
      continue;
    }
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
      port->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (port->sock == -1)
        continue;
      if ((opt = fcntl(port->sock, F_GETFL, 0)) < 0)
        continue;
      fcntl(port->sock, F_SETFL, opt | O_NONBLOCK);
      opt = 1;
      setsockopt(port->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      if (bind(port->sock, rp->ai_addr, rp->ai_addrlen) == 0) {
        if (listen(port->sock, 5) == 0) /* TODO: hardcoded */
          break; /* success */
        close(port->sock);
        port->sock = -1;
      }
    }
    freeaddrinfo(result);
    if (port->sock < 0)
      log_msg(cfg, error, "can't bind/listen on %s:%s", port->host, port->port);
  }

  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  for (f2b_port_t *port = cfg->ports; port != NULL; port = port->next)
    close(port->sock);

  return true;
}

bool
next(cfg_t *cfg, char *buf, size_t bufsize, bool reset) {
  struct sockaddr_storage addr;
  socklen_t addrlen;

  assert(cfg != NULL);
  assert(buf != NULL);
  assert(bufsize > 0);

  if (reset || cfg->current == NULL)
    cfg->current = cfg->ports;

  for (f2b_port_t *port = cfg->current; port != NULL; port = port->next) {
    if (port->sock < 0)
      continue;
    addrlen = sizeof(addr);
    int sock = accept(port->sock, (struct sockaddr *) &addr, &addrlen);
    if (sock < 0 && errno == EAGAIN)
      continue;
    if (sock < 0) {
      log_msg(cfg, error, "accept() error: %s", strerror(errno));
      continue;
    }
    port->accepts++;
    shutdown(sock, SHUT_RDWR);
    if (addr.ss_family == AF_INET) {
      inet_ntop(AF_INET,  &(((struct sockaddr_in *) &addr)->sin_addr), buf, bufsize);
      return true;
    }
    if (addr.ss_family == AF_INET6) {
      inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) &addr)->sin6_addr), buf, bufsize);
      return true;
    }
    cfg->logcb(error, "can't convert sockaddr to string: unknown AF");
  }

  return false;
}

bool
stats(cfg_t *cfg, char *buf, size_t bufsize) {
  char tmp[256];
  const char *fmt =
    "- listen: %s:%s\n"
    "  accepts: %u\n";
  assert(cfg != NULL);

  if (buf == NULL || bufsize == 0)
    return false;

  for (f2b_port_t *p = cfg->ports; p != NULL; p = p->next) {
    snprintf(tmp, sizeof(tmp), fmt, p->host, p->port, p->accepts);
    strlcat(buf, tmp, bufsize);
  }

  return true;
}

void
destroy(cfg_t *cfg) {
  f2b_port_t *next;
  assert(cfg != NULL);

  for (; cfg->ports != NULL; cfg->ports = next) {
    next = cfg->ports->next;
    free(cfg->ports);
  }

  free(cfg);
}
