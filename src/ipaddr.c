#include "common.h"
#include "ipaddr.h"

f2b_ipaddr_t *
f2b_ipaddr_create(const char *addr, size_t matches) {
  f2b_ipaddr_t *a = NULL;

  assert(addr != NULL);
  assert(matches != 0);

  if ((a = calloc(1, sizeof(f2b_ipaddr_t))) != NULL) {
    strncpy(a->text, addr, sizeof(a->text));
    if (strchr(addr, ':') == NULL) {
      a->type = AF_INET;
      if (inet_pton(a->type, addr, &a->binary.v4) < 1)
        goto cleanup;
    } else {
      a->type = AF_INET6;
      if (inet_pton(a->type, addr, &a->binary.v6) < 1)
        goto cleanup;
    }
    
    if (f2b_matches_create(&a->matches, matches) == false)
      goto cleanup;
  }
  return a;

  cleanup:
  FREE(a);
  return NULL;
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

  for (f2b_ipaddr_t *a = list; a->next != NULL; a = a->next) {
    if (strncmp(a->text, addr, sizeof(a->text)) == 0)
      return a;
  }

  return NULL;
}
