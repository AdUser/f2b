#include "../src/common.h"
#include "../src/matches.h"
#include "../src/ipaddr.h"

#include <arpa/inet.h>

int main() {
  bool res = false;
  f2b_ipaddr_t *list = NULL;
  f2b_ipaddr_t *addr = NULL;

  UNUSED(res);
  UNUSED(addr);
  UNUSED(list);

  addr = f2b_addrlist_lookup(list, "127.0.0.1");
  assert(addr == NULL);

  addr = f2b_ipaddr_create("400.400.400.400");
  assert(addr == NULL);
  addr = f2b_ipaddr_create("127.0.0.1");
  assert(addr != NULL);

  assert(addr->type == AF_INET);
  assert(addr->next == NULL);
  assert(strncmp(addr->text, "127.0.0.1", sizeof(addr->text)) == 0);
  assert(addr->matches.list != NULL);
  assert(addr->matches.count == 0);

  list = f2b_addrlist_append(list, addr);
  assert(list != NULL);
  assert(f2b_addrlist_lookup(list, "127.0.0.1") != NULL);
  assert(list == addr);

  list = f2b_addrlist_remove(list, "127.4.4.4");
  assert(list != NULL);
  list = f2b_addrlist_remove(list, "127.0.0.1");
  assert(list == NULL);

  assert((addr = f2b_ipaddr_create("127.0.0.1")) != NULL);
  assert((list = f2b_addrlist_append(list, addr)) != NULL);
  assert((addr = f2b_ipaddr_create("127.0.0.2")) != NULL);
  assert((list = f2b_addrlist_append(list, addr)) != NULL);
  assert((addr = f2b_ipaddr_create("127.0.0.3")) != NULL);
  assert((list = f2b_addrlist_append(list, addr)) != NULL);
  assert((addr = f2b_ipaddr_create("127.0.0.4")) != NULL);
  assert((list = f2b_addrlist_append(list, addr)) != NULL);
  assert((addr = f2b_ipaddr_create("127.0.0.5")) != NULL);
  assert((list = f2b_addrlist_append(list, addr)) != NULL);

  assert((list = f2b_addrlist_remove(list, "127.0.0.2")) != NULL);
  assert((list = f2b_addrlist_remove(list, "127.0.0.4")) != NULL);
  assert((list = f2b_addrlist_remove(list, "127.0.0.3")) != NULL);
  assert((list = f2b_addrlist_remove(list, "127.0.0.5")) != NULL);
  assert((list = f2b_addrlist_remove(list, "127.0.0.1")) == NULL);

  return 0;
}
