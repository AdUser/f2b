#include "../src/common.h"
#include "../src/matches.h"

int main() {
  f2b_match_t *match = NULL;
  f2b_matches_t matches;
  bool result = false;
  size_t size = 15;
  time_t now  = time(NULL);

  UNUSED(result);

  memset(&matches, 0x0, sizeof(matches));

  match = f2b_match_create(now);
  assert(match != NULL);
  assert(match->next == NULL);

  f2b_matches_prepend(&matches, match);
  assert(matches.count == 1);
  assert(matches.last == now);
  assert(matches.list == match);

  f2b_matches_expire(&matches, now + 1);
  assert(matches.count == 0);
  assert(matches.last == 0);
  assert(matches.list == NULL);

  for (size_t i = 1; i < size; i++) {
    match = f2b_match_create(now - 60 * (size - i));
    f2b_matches_prepend(&matches, match);
    assert(matches.count == i);
    assert(matches.last == now - (time_t) (60 * (size - i)));
  }

  f2b_matches_expire(&matches, now - 60 * 4);
  assert(matches.count == 3);

  f2b_matches_flush(&matches);
  assert(matches.count == 0);
  assert(matches.last == 0);
  assert(matches.list == NULL);

  return 0;
}
