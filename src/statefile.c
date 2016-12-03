/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "common.h"
#include "log.h"
#include "ipaddr.h"
#include "statefile.h"

f2b_statefile_t *
f2b_statefile_create(const char *statedir, const char *jailname) {
  f2b_statefile_t *sf = NULL;
  char path[PATH_MAX];

  assert(statedir != NULL);
  assert(jailname != NULL);

  snprintf(path, sizeof(path), "%s/%s.state", statedir, jailname);

  if (access(path, R_OK | W_OK) < 0) {
    if (errno != ENOENT) {
      f2b_log_msg(log_error, "can't access statefile: %s", strerror(errno));
      return NULL;
    }
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP; /* 0640 */
    int fd = open(path, O_CREAT | O_WRONLY, mode);
    if (fd < 0) {
      f2b_log_msg(log_error, "can't create statefile: %s", strerror(errno));
      return NULL;
    }
    close(fd);
  }

  if ((sf = calloc(1, sizeof(f2b_statefile_t))) == NULL) {
    f2b_log_msg(log_error, "can't allocate memory for statefile: %s", strerror(errno));
    return NULL;
  }

  strlcpy(sf->path, path, sizeof(sf->path));

  return sf;
}

void
f2b_statefile_destroy(f2b_statefile_t *sf) {
  if (!sf) return;
  free(sf);
}

f2b_ipaddr_t *
f2b_statefile_load(f2b_statefile_t *sf, size_t matches) {
  const int fields = 3;
  const char *format = "%48s %lu %lu"; /* 48 == IPADDR_MAX == sizeof(addr) */
  f2b_ipaddr_t *addrlist = NULL, *ipaddr = NULL;
  char buf[256], addr[IPADDR_MAX], *p;
  int banned, release;
  FILE *f = NULL;

  assert(sf != NULL);

  if ((f = fopen(sf->path, "r")) == NULL) {
    f2b_log_msg(log_error, "can't open statefile: %s", strerror(errno));
    return NULL;
  }

  while(fgets(buf, sizeof(buf), f)) {
    p = &buf[0];
    while(isspace(*p))
      p++; /* skip leading spaces */
    if (*p == '#')
      continue; /* is comment */
    if (sscanf(buf, format, addr, &banned, &release) != fields) {
      f2b_log_msg(log_warn, "can't parse, ignoring line: %s", buf);
      continue;
    }
    if ((ipaddr = f2b_ipaddr_create(addr, matches)) == NULL) {
      f2b_log_msg(log_warn, "can't parse addr: %s", addr);
      continue;
    }
    ipaddr->banned = true;
    ipaddr->banned_at  = banned;
    ipaddr->release_at = release;
    ipaddr->next = addrlist;
    addrlist = ipaddr;
  }

  fclose(f);
  return addrlist;
}

bool
f2b_statefile_save(f2b_statefile_t *sf, f2b_ipaddr_t *addrlist) {
  const char *fmt = "%s\t%lu\t%lu\n";
  char path[PATH_MAX];
  FILE *f = NULL;

  assert(sf != NULL);

  if (addrlist == NULL)
    return true;

  strlcpy(path, sf->path, sizeof(path));
  strlcat(path, ".new",   sizeof(path));

  if ((f = fopen(path, "w")) == NULL) {
    f2b_log_msg(log_error, "can't open statefile: %s", strerror(errno));
    return false;
  }

  fprintf(f, "# address\tbanned_at\trelease_at\n");
  for (f2b_ipaddr_t *ipaddr = addrlist; ipaddr != NULL; ipaddr = ipaddr->next) {
    if (!ipaddr->banned)
      continue;
    fprintf(f, fmt, ipaddr->text, ipaddr->banned_at, ipaddr->release_at);
  }
  fclose(f);

  if (rename(path, sf->path) < 0) {
    f2b_log_msg(log_error, "can't replace statefile %s: %s", sf->path, strerror(errno));
    return false;
  }

  sf->need_save = false;
  return true;
}
