/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "matches.h"
#include "ipaddr.h"

#include <arpa/inet.h>

f2b_ipaddr_t *
f2b_ipaddr_create(const char *addr) {
  f2b_ipaddr_t *a = NULL;

  assert(addr != NULL);

  if ((a = calloc(1, sizeof(f2b_ipaddr_t))) != NULL) {
    strlcpy(a->text, addr, sizeof(a->text));
    if (strchr(addr, ':') == NULL) {
      a->type = AF_INET;
      if (inet_pton(a->type, addr, &a->binary) < 1)
        goto cleanup;
    } else {
      a->type = AF_INET6;
      if (inet_pton(a->type, addr, &a->binary) < 1)
        goto cleanup;
    }
  }
  return a;

  cleanup:
  free(a);
  return NULL;
}

void
f2b_ipaddr_destroy(f2b_ipaddr_t *ipaddr) {
  assert(ipaddr != NULL);

  f2b_matches_flush(&ipaddr->matches);
  free(ipaddr);
}

void
f2b_ipaddr_status(f2b_ipaddr_t *addr, char *res, size_t ressize) {
  struct tm t;
  char buf[2048], stime[32] = "", btime1[32] = "", btime2[32] = "";
  assert(addr != NULL);
  assert(res != NULL);
  const char *fmt_dt = "%Y-%m-%d %H:%M:%S";
  const char *fmt_status =
    "ipaddr: %s\n"
    "lastseen: %s\n"
    "bancount: %u\n";
  const char *fmt_ban =
    "banned: yes\n"
    "bantime:\n"
    "  from: %s\n"
    "  to:   %s\n";
  const char *fmt_match =
    "  - time: %s, score: %d, stag: %08X, ftag: %08X\n";
  strftime(stime, sizeof(stime), fmt_dt, localtime(&addr->lastseen));
  snprintf(res, ressize, fmt_status, addr->text, stime, addr->bancount);
  if (addr->banned) {
    strftime(btime1, sizeof(btime1), fmt_dt, localtime_r(&addr->banned_at,  &t));
    strftime(btime2, sizeof(btime2), fmt_dt, localtime_r(&addr->release_at, &t));
    snprintf(buf, sizeof(buf), fmt_ban, btime1, btime2);
    strlcat(res, buf, ressize);
  } else {
    strlcat(res, "banned: no\n", ressize);
    if (addr->matches.count) {
      strlcat(res, "matches:\n", ressize);
      for (f2b_match_t *m = addr->matches.list; m != NULL; m = m->next) {
        strftime(stime, sizeof(stime), fmt_dt, localtime_r(&m->time,  &t));
        snprintf(buf, sizeof(buf), fmt_match, stime, m->score, m->stag, m->ftag);
        strlcat(res, buf, ressize);
      }
    }
  }
}

f2b_ipaddr_t *
f2b_addrlist_append(f2b_ipaddr_t *list, f2b_ipaddr_t *ipaddr) {
  assert(ipaddr != NULL);

  ipaddr->next = list;
  return ipaddr;
}

f2b_ipaddr_t *
f2b_addrlist_lookup(f2b_ipaddr_t *list, const char *addr) {
  assert(addr != NULL);

  if (list == NULL)
    return NULL;

  for (f2b_ipaddr_t *a = list; a != NULL; a = a->next) {
    if (strncmp(a->text, addr, sizeof(a->text)) == 0)
      return a;
  }

  return NULL;
}

f2b_ipaddr_t *
f2b_addrlist_remove(f2b_ipaddr_t *list, const char *addr) {
  f2b_ipaddr_t *a = NULL, *prev = NULL;

  assert(addr != NULL);

  for (a = list; a != NULL; a = a->next) {
    if (strncmp(a->text, addr, sizeof(a->text)) == 0) {
      if (prev == NULL) {
        /* first elem in list */
        list = a->next;
      } else {
        /* somewhere in list */
        prev->next = a->next;
      }
      f2b_ipaddr_destroy(a);
      return list;
    }
    prev = a;
  }

  return list;
}

f2b_ipaddr_t *
f2b_addrlist_destroy(f2b_ipaddr_t *list) {
  f2b_ipaddr_t *next = NULL;

  for (; list != NULL; list = next) {
    next = list->next;
    f2b_ipaddr_destroy(list);
  }

  return NULL;
}
