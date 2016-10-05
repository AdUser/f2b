/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "source.h"

#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define HOST_MAX 48
#define PORT_MAX 6

typedef struct f2b_port_t {
  struct f2b_port_t *next;
  char host[HOST_MAX];
  char port[PORT_MAX];
  int sock;
} f2b_port_t;

struct _config {
  char name[32];
  char error[256];
  void (*errcb)(char *errstr);
  f2b_port_t *ports;
  f2b_port_t *current;
};

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
    f2b_port_t *port = NULL;
    if ((port = calloc(1, sizeof(f2b_port_t))) == NULL) {
      strlcpy(cfg->error, "out of memory", sizeof(cfg->error));
      return false;
    }
    if (try_parse_listen_opt(port, value) == false) {
      snprintf(cfg->error, sizeof(cfg->error), "can't parse: %s", value);
      free(port);
      return false;
    }
    port->next = cfg->ports;
    cfg->ports = port;
    return true;
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
  struct addrinfo hints;
  struct addrinfo *result;
  assert(cfg != NULL);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  for (f2b_port_t *port = cfg->ports; port != 0; port = port->next) {
    port->sock = -1;
    int ret = getaddrinfo(port->host, port->port, &hints, &result);
    int opt = 1;
    if (ret != 0) {
      snprintf(cfg->error, sizeof(cfg->error), "getaddrinfo: %s", gai_strerror(ret));
      cfg->errcb(cfg->error);
      continue;
    }
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
      port->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (port->sock == -1)
        continue;
      setsockopt(port->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      if (bind(port->sock, rp->ai_addr, rp->ai_addrlen) == 0) {
        if (listen(port->sock, 5) == 0) /* TODO: hardcoded */
          break; /* success */
        close(port->sock);
        port->sock = -1;
      }
    }
    freeaddrinfo(result);
    if (port->sock < 0) {
      snprintf(cfg->error, sizeof(cfg->error), "can't bind/listen on %s:%s", port->host, port->port);
      cfg->errcb(cfg->error);
    }
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

  for (; cfg->ports != NULL; cfg->ports = next) {
    next = cfg->ports->next;
    free(cfg->ports);
  }

  free(cfg);
}
