/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <regex.h>

#include "../strlcpy.h"
#include "filter.h"

#define MODNAME "preg"
/* draft */
#define HOST_REGEX "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})"

struct _regexp {
  rx_t *next;
  char pattern[PATTERN_MAX];
  int matches;
  regex_t regex;
};

struct _config {
  char id[ID_MAX];
  int flags;
  bool icase;
  void (*logcb)(enum loglevel lvl, const char *msg);
  rx_t *regexps;
};

#include "filter.c"

cfg_t *
create(const char *id) {
  cfg_t *cfg = NULL;

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  strlcpy(cfg->id, id, sizeof(cfg->id));

  cfg->logcb = &logcb_stub;
  cfg->flags |= MOD_TYPE_FILTER;
  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg   != NULL);
  assert(key   != NULL);
  assert(value != NULL);

  if (strcmp(key, "icase") == 0) {
    cfg->icase = (strcmp(value, "yes") == 0) ? true : false;
    return true;
  }

  return false;
}

bool
append(cfg_t *cfg, const char *pattern) {
  rx_t *regex = NULL;
  int flags = REG_EXTENDED;
  size_t bufsize;
  char *buf = NULL;
  char *token = NULL;
  int ret;

  assert(pattern != NULL);

  if (cfg->icase)
    flags |= REG_ICASE;

  if ((token = strstr(pattern, HOST_TOKEN)) == NULL)
    return false;

  bufsize = strlen(pattern) + strlen(HOST_REGEX) + 1;
  if ((buf = alloca(bufsize)) == NULL)
    return false;

  memset(buf, 0x0, bufsize);
  memcpy(buf, pattern, token - pattern);
  strlcat(buf, HOST_REGEX, bufsize);
  strlcat(buf, token + strlen(HOST_TOKEN), bufsize);

  if ((regex = calloc(1, sizeof(rx_t))) == NULL)
    return false;

  if ((ret = regcomp(&regex->regex, buf, flags)) == 0) {
    regex->next = cfg->regexps;
    cfg->regexps = regex;
    strlcpy(regex->pattern, pattern, sizeof(regex->pattern));
    cfg->flags |= MOD_IS_READY;
    return true;
  } else {
    char buf[256] = "";
    regerror(ret, &regex->regex, buf, sizeof(buf));
    log_msg(cfg, error, "regex compile error: %s", buf);
  }

  free(regex);
  return false;
}

bool
match(cfg_t *cfg, const char *line, char *buf, size_t buf_size) {
  size_t match_len = 0;
  regmatch_t match[2];

  assert(cfg  != NULL);
  assert(line != NULL);
  assert(buf  != NULL);

  for (rx_t *r = cfg->regexps; r != NULL; r = r->next) {
    if (regexec(&r->regex, line, 2, &match[0], 0) != 0)
      continue;
    /* matched */
    r->matches++;
    match_len = match[1].rm_eo - match[1].rm_so;
    assert(buf_size > match_len);
    memcpy(buf, &line[match[1].rm_so], match_len);
    buf[match_len] = '\0';
    buf[buf_size - 1]  = '\0';
    return true;
  }

  return false;
}

void
flush(cfg_t *cfg) {
  rx_t *next = NULL;

  assert(cfg != NULL);

  for (rx_t *r = cfg->regexps; r != NULL; r = next) {
    next = r->next;
    regfree(&r->regex);
    free(r);
  }
  cfg->regexps = NULL;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  flush(cfg);
  free(cfg);
}
