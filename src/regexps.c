/* this file should not be used directly, only with `#include "regexps.c"` */

#include "log.h"

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
f2b_regexlist_from_file(const char *path) {
  f2b_regex_t *list = NULL, *regex = NULL;
  FILE *f = NULL;
  size_t linenum = 0;
  char line[REGEX_LINE_MAX] = "";
  char *p, *q;

  if ((f = fopen(path, "r")) == NULL) {
    f2b_log_msg(log_error, "can't open regex list '%s': %s", path, strerror(errno));
    return NULL;
  }

  while (1) {
    p = fgets(line, sizeof(line), f);
    if (!p && (feof(f) || ferror(f)))
      break;
    linenum++;
    /* strip leading spaces */
    while (isblank(*p))
      p++;
    /* strip trailing spaces */
    if ((q = strchr(p, '\r')) || (q = strchr(p, '\n'))) {
      while(q > p && isspace(*q)) {
        *q = '\0';
        q--;
      }
    }
    switch(*p) {
      case '\r':
      case '\n':
      case '\0':
        /* empty line */
        break;
      case ';':
      case '#':
        /* comment line */
        break;
      default:
        /* TODO: icase */
        if ((regex = f2b_regex_create(p, false)) == NULL) {
          f2b_log_msg(log_warn, "can't create regex from pattern at %s:%d: %s", path, linenum, p);
          continue;
        }
        list = f2b_regexlist_append(list, regex);
        break;
    }
  }

  return list;
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
