#include "../src/common.h"
#include "../src/log.h"
#include "../src/ipaddr.h"
#include "../src/filters/filter.h"

int main() {
  cfg_t *filter = NULL;
  char matchbuf[IPADDR_MAX] = "";
  bool result = false;

  UNUSED(result);

  filter = create("test");
  assert(filter != NULL);

  result = config(filter, "nonexistent", "yes");
  assert(result == false);

  result = config(filter, "icase", "yes");
  assert(result == true);

  result = config(filter, "icase", "no");
  assert(result == true);

  result = ready(filter);
  assert(result == false);

  result = append(filter, "host without marker");
  assert(result == false);

  result = append(filter, "host with marker <HOST>");
  assert(result == true);

  result = ready(filter);
  assert(result == true);

  result = match(filter, "host", matchbuf, sizeof(matchbuf));
  assert(result == false);

  result = match(filter, "host with marker <HOST>", matchbuf, sizeof(matchbuf));
  assert(result == false);

  result = match(filter, "host with marker 1.2.3.4", matchbuf, sizeof(matchbuf));
  assert(result == true);

  result = match(filter, "host with marker example.com", matchbuf, sizeof(matchbuf));
  assert(result == false);

  destroy(filter);

  return 0;
}
