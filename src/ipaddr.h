#ifndef F2B_IPADDR_H_
#define F2B_IPADDR_H_

#include <arpa/inet.h>

#include "matches.h"

typedef struct f2b_ipaddr_t {
  struct f2b_ipaddr_t *next;
  int type;
  char text[40]; /* 8 x "ffff" + 7 x ":" + '\0' */
  union {
    struct in_addr  v4;
    struct in6_addr v6;
  } binary;
  f2b_matches_t matches;
} f2b_ipaddr_t;

f2b_ipaddr_t * f2b_ipaddr_create(const char *addr, size_t max_matches);
f2b_ipaddr_t * f2b_addrlist_append(f2b_ipaddr_t *list, f2b_ipaddr_t *ipaddr);
f2b_ipaddr_t * f2b_addrlist_lookup(f2b_ipaddr_t *list, const char *addr);

#endif /* F2B_IPADDR_H_ */
