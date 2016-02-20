#ifndef F2B_IPADDR_H_
#define F2B_IPADDR_H_

#include <arpa/inet.h>

#include "matches.h"

typedef struct {
  struct f2b_ipaddr_t *next;
  char text[40]; /* 8 x "ffff" + 7 x ":" + '\0' */
  int addr_type;
  union {
    struct in_addr  v4;
    struct in6_addr v6;
  } binary;
  f2b_matches_t matches;
} f2b_ipaddr_t;

#endif /* F2B_IPADDR_H_ */
