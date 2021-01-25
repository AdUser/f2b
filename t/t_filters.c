#include "../src/common.h"
#include "../src/log.h"
#include "../src/matches.h"
#include "../src/ipaddr.h"
#include "../src/filters/filter.h"

int main() {
  cfg_t *filter = NULL;
  char matchbuf[IPADDR_MAX] = "";
  bool result = false;
  short int score;
  int flags;
  uint32_t ftag;

  UNUSED(result);

  filter = create("test");
  assert(filter != NULL);

  result = config(filter, "nonexistent", "yes");
  assert(result == false);

  result = config(filter, "icase", "yes");
  assert(result == true);

  result = config(filter, "icase", "no");
  assert(result == true);

  flags = state(filter);
  assert((flags & MOD_IS_READY) == 0);

  result = append(filter, "host without marker");
  assert(result == false);

  result = append(filter, "host with marker <HOST>");
  assert(result == true);

  flags = state(filter);
  assert(flags & MOD_IS_READY);

  ftag = match(filter, "host", matchbuf, sizeof(matchbuf), &score);
  assert(ftag == 0);

  ftag = match(filter, "host with marker <HOST>", matchbuf, sizeof(matchbuf), &score);
  assert(ftag == 0);

  ftag = match(filter, "host with marker 1.2.3.4", matchbuf, sizeof(matchbuf), &score);
  assert(ftag > 0);

  ftag = match(filter, "host with marker example.com", matchbuf, sizeof(matchbuf), &score);
  assert(ftag == 0);

  destroy(filter);

  return 0;
}
