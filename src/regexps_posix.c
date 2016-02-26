#include "common.h"
#include "regexps.h"

#include <regex.h>

/* draft */
#define HOST_REGEX "([12][0-9]{0,2}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})"

struct _regex {
  struct _regex *next;
  int matches;
  regex_t regex;
};

f2b_regex_t *
f2b_regex_create(const char *pattern, bool icase) {
  f2b_regex_t *regex = NULL;
  int flags = REG_EXTENDED;
  size_t bufsize;
  char *buf = NULL;
  char *token = NULL;

  assert(pattern != NULL);

  if (icase)
    flags |= REG_ICASE;

  if ((token = strstr(pattern, HOST_TOKEN)) == NULL)
    return NULL;

  bufsize = strlen(pattern) + strlen(HOST_REGEX) + 1;
  if ((buf = alloca(bufsize)) == NULL)
    return NULL;

  memset(buf, 0x0, bufsize);
  strncpy(buf, pattern, token - pattern);
  strcat(buf, HOST_REGEX);
  strcat(buf, token + strlen(HOST_TOKEN));

  if ((regex = calloc(1, sizeof(f2b_regex_t))) == NULL)
    return NULL;

  if (regcomp(&regex->regex, buf, flags) == 0)
    return regex;

  FREE(regex);
  return NULL;
}

void
f2b_regex_destroy(f2b_regex_t * regex) {
  regfree(&regex->regex);
  FREE(regex);
}

f2b_regex_t *
f2b_regexlist_append(f2b_regex_t *list, f2b_regex_t *regex) {
  assert(regex != NULL);

  regex->next = list;
  return regex;
}

bool
f2b_regexlist_match(f2b_regex_t *list, const char *line, char *matchbuf, size_t matchbuf_size) {
  size_t match_len = 0;
  regmatch_t match[2];

  for (; list != NULL; list = list->next) {
    if (regexec(&list->regex, line, 2, &match[0], 0) != 0)
      continue;
    list->matches++;
    match_len = match[1].rm_eo - match[1].rm_so;
    assert(matchbuf_size > match_len);
    strncpy(matchbuf, &line[match[1].rm_so], match_len);
    matchbuf[match_len] = '\0';
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
