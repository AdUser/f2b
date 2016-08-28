/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_IPADDR_H_
#define F2B_IPADDR_H_

#include <arpa/inet.h>

#include "matches.h"

#define IPADDR_MAX 48 /* 8 x "ffff" + 7 x "::" + '\0' */

typedef struct f2b_ipaddr_t {
  struct f2b_ipaddr_t *next;
  int type;
  char text[IPADDR_MAX];
  bool banned;
  size_t bancount;
  time_t lastseen;
  time_t banned_at;
  time_t release_at;
  union {
    struct in_addr  v4;
    struct in6_addr v6;
  } binary;
  f2b_matches_t matches;
} f2b_ipaddr_t;

f2b_ipaddr_t * f2b_ipaddr_create (const char *addr, size_t max_matches);
void           f2b_ipaddr_destroy(f2b_ipaddr_t *ipaddr);
void           f2b_ipaddr_status (f2b_ipaddr_t *ipaddr, char *res, size_t ressize);

f2b_ipaddr_t * f2b_addrlist_append(f2b_ipaddr_t *list, f2b_ipaddr_t *ipaddr);
f2b_ipaddr_t * f2b_addrlist_lookup(f2b_ipaddr_t *list, const char *addr);
f2b_ipaddr_t * f2b_addrlist_remove(f2b_ipaddr_t *list, const char *addr);
f2b_ipaddr_t * f2b_addrlist_destroy(f2b_ipaddr_t *list);

#endif /* F2B_IPADDR_H_ */
