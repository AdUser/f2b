#include "../src/common.h"
#include "../src/matches.h"

int main() {
  f2b_matches_t matches;
  size_t size = 15;
  time_t now  = time(NULL);

  assert(f2b_matches_create(&matches, size) == true);
  assert(matches.used == 0);
  assert(matches.max == 15);
  assert(matches.times != NULL);

  for (size_t i = 0; i < size; i++)
    assert(f2b_matches_append(&matches, now - 60 * i) == true);

  assert(f2b_matches_append(&matches, 0) == false);

  f2b_matches_expire(&matches, now - 60 * 4);
  assert(matches.used == 4);
  assert(matches.max == 15);

  f2b_matches_destroy(&matches);
  assert(matches.used == 0);
  assert(matches.max  == 0);
  assert(matches.times == NULL);

  return 0;
}
