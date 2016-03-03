/* this file should not be used directly, only with `#include "regexps.c"` */

f2b_regex_t *
f2b_regexlist_append(f2b_regex_t *list, f2b_regex_t *regex) {
  assert(regex != NULL);

  regex->next = list;
  return regex;
}

bool
f2b_regexlist_match(f2b_regex_t *list, const char *line, char *buf, size_t buf_size) {
  for (; list != NULL; list = list->next) {
    if (f2b_regex_match(list, line, buf, buf_size))
      return true;
  }

  return false;
}

f2b_regex_t *
f2b_regexlist_destroy(f2b_regex_t *list) {
  f2b_regex_t *next;

  for (; list != NULL; list = next) {
    next = list->next;
    f2b_regex_destroy(list);
  }

  return NULL;
}
