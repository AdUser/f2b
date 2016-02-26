#include "../src/common.h"
#include "../src/regexps.h"

int main() {
  f2b_regex_t *list  = NULL;
  f2b_regex_t *regex = NULL;
  char matchbuf[32] = "";

  assert((regex = f2b_regex_create("line without HOST", true)) == NULL);
  assert((regex = f2b_regex_create("line with <HOST>",  true)) != NULL);

  assert(f2b_regexlist_match(list, "line with 192.168.0.1", matchbuf, sizeof(matchbuf)) == false);

  assert((list = f2b_regexlist_append(list, regex)) != NULL);

  assert(f2b_regexlist_match(list, "line with 192.168.0.1", matchbuf, sizeof(matchbuf)) == true);
  assert(strncmp(matchbuf, "192.168.0.1", sizeof(matchbuf)) == 0);

  assert((list = f2b_regexlist_destroy(list)) == NULL);

  return 0;
}
