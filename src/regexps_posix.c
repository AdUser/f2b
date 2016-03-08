#include "common.h"
#include "regexps.h"

#include <regex.h>

/* draft */
#define HOST_REGEX "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})"

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

bool
f2b_regex_match(f2b_regex_t *regex, const char *line, char *buf, size_t buf_size) {
  size_t match_len = 0;
  regmatch_t match[2];

  assert(regex != NULL);
  assert(line  != NULL);
  assert(buf   != NULL);

  if (regexec(&regex->regex, line, 2, &match[0], 0) != 0)
    return false;

  regex->matches++;
  match_len = match[1].rm_eo - match[1].rm_so;
  assert(buf_size > match_len);
  strncpy(buf, &line[match[1].rm_so], match_len);
  buf[match_len] = '\0';

  return true;
}

/* common f2b_regexlist_*() functions */
#include "regexps.c"
