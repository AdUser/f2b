/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "ipaddr.h"

f2b_ipaddr_t *
f2b_ipaddr_create(const char *addr) {
  f2b_ipaddr_t *a = NULL;

  assert(addr != NULL);

  if ((a = calloc(1, sizeof(f2b_ipaddr_t))) != NULL) {
    strlcpy(a->text, addr, sizeof(a->text));
    if (strchr(addr, ':') == NULL) {
      a->type = AF_INET;
      if (inet_pton(a->type, addr, &a->binary.v4) < 1)
        goto cleanup;
    } else {
      a->type = AF_INET6;
      if (inet_pton(a->type, addr, &a->binary.v6) < 1)
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
  assert(addr != NULL);
  assert(res != NULL);
  const char *fmt =
    "ipaddr: %s\n"
    "banned: %s\n"
    "bancount: %u\n"
    "matches: %u\n"
    "lastseen: %u\n"
    "banned_at: %u\n"
    "release_at: %u\n";
  snprintf(res, ressize, fmt, addr->text, addr->banned ? "yes" : "no",
    addr->bancount, addr->matches.count, addr->lastseen, addr->banned_at, addr->release_at);
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
