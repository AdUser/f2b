#include "../src/common.h"
#include "../src/ipaddr.h"

int main() {
  f2b_ipaddr_t *list = NULL;
  f2b_ipaddr_t *addr = NULL;

  assert(f2b_addrlist_lookup(list, "127.0.0.1") == NULL);

  assert((addr = f2b_ipaddr_create("400.400.400.400", 15)) == NULL);
  assert((addr = f2b_ipaddr_create("127.0.0.1",       15)) != NULL);

  assert(addr->type == AF_INET);
  assert(addr->next == NULL);
  assert(strncmp(addr->text, "127.0.0.1", sizeof(addr->text)) == 0);
  assert(addr->matches.times != NULL);
  assert(addr->matches.max == 15);
  assert(addr->matches.used == 0);

  assert(0); /* TODO: add more ips to list & test this cases */

  assert((list = f2b_addrlist_append(list, addr)) != NULL);
  assert(f2b_addrlist_lookup(list, "127.0.0.1") != NULL);
  assert(list == addr);

  assert((list = f2b_addrlist_remove(list, "127.4.4.4")) != NULL);
  assert((list = f2b_addrlist_remove(list, "127.0.0.1")) == NULL);

  return 0;
}
