/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <alloca.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filter.h"

#include <regex.h>

/* draft */
#define HOST_REGEX "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})"

typedef struct f2b_regex_t {
  struct f2b_regex_t *next;
  int matches;
  regex_t regex;
} f2b_regex_t;

struct _config {
  char id[32];
  char error[256];
  bool icase;
  f2b_regex_t *regexps;
};

cfg_t *
create(const char *id) {
  cfg_t *cfg = NULL;

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  snprintf(cfg->id, sizeof(cfg->id), "%s", id);

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
  f2b_regex_t *regex = NULL;
  int flags = REG_EXTENDED;
  size_t bufsize;
  char *buf = NULL;
  char *token = NULL;

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
  strcat(buf, HOST_REGEX);
  strcat(buf, token + strlen(HOST_TOKEN));

  if ((regex = calloc(1, sizeof(f2b_regex_t))) == NULL)
    return false;

  if (regcomp(&regex->regex, buf, flags) == 0) {
    regex->next = cfg->regexps;
    cfg->regexps = regex;
    return true;
  }

  free(regex);
  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);
  if (cfg->regexps)
    return true;
  return false;
}

const char *
error(cfg_t *cfg) {
  assert(cfg != NULL);

  return cfg->error;
}

bool
match(cfg_t *cfg, const char *line, char *buf, size_t buf_size) {
  f2b_regex_t *r = NULL;
  size_t match_len = 0;
  regmatch_t match[2];

  assert(cfg  != NULL);
  assert(line != NULL);
  assert(buf  != NULL);

  for (r = cfg->regexps; r != NULL; r = r->next) {
    if (regexec(&r->regex, line, 2, &match[0], 0) != 0)
      continue;
    /* matched */
    r->matches++;
    match_len = match[1].rm_eo - match[1].rm_so;
    assert(buf_size > match_len);
    memcpy(buf, &line[match[1].rm_so], match_len);
    buf[match_len] = '\0';
    buf[buf_size]  = '\0';
    return true;
  }

  return false;
}

void
destroy(cfg_t *cfg) {
  f2b_regex_t *next = NULL, *r = NULL;

  for (r = cfg->regexps; r != NULL; r = next) {
    next = r->next;
    regfree(&r->regex);
    free(r);
  }
  free(cfg);
}
