/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "filter.h"

#include <pcre.h>

#define HOST_REGEX "(?<host>[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})"

typedef struct f2b_regex_t {
  struct f2b_regex_t *next;
  char pattern[PATTERN_MAX];
  int matches;
  pcre *regex;
  pcre_extra *data;
} f2b_regex_t;

struct _config {
  char id[ID_MAX];
  char error[256];
  bool icase;
  bool study;
  bool usejit;
  f2b_regex_t *regexps;
  f2b_regex_t *statp;
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
  if (strcmp(key, "study") == 0) {
    cfg->study = (strcmp(value, "yes") == 0) ? true : false;
    return true;
  }
  if (strcmp(key, "usejit") == 0) {
    cfg->usejit = (strcmp(value, "yes") == 0) ? true : false;
    return true;
  }

  return false;
}

bool
append(cfg_t *cfg, const char *pattern) {
  f2b_regex_t *regex = NULL;
  int flags = 0;
  size_t bufsize;
  char *buf = NULL;
  char *token = NULL;
  const char *errptr = NULL;
  int erroffset = 0;

  assert(pattern != NULL);

  if (cfg->icase)
    flags |= PCRE_CASELESS;

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

  if ((regex->regex = pcre_compile(buf, flags, &errptr, &erroffset, NULL)) == NULL) {
    snprintf(cfg->error, sizeof(cfg->error), "regex compilation failed at %d: %s", erroffset, errptr);
    free(regex);
    return false;
  }

  if (cfg->study) {
    flags = 0;
    if (cfg->usejit)
      flags |= PCRE_STUDY_JIT_COMPILE;
    if ((regex->data = pcre_study(regex->regex, 0, &errptr)) == NULL) {
      snprintf(cfg->error, sizeof(cfg->error), "regex learn failed: %s", errptr);
      pcre_free(regex->regex);
      free(regex);
      return false;
    }
  }

  regex->next = cfg->regexps;
  cfg->regexps = regex;
  snprintf(regex->pattern, sizeof(regex->pattern), "%s", pattern);
  return true;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);
  if (cfg->regexps)
    return true;
  return false;
}

bool
stats(cfg_t *cfg, int *matches, char **pattern, bool reset) {
  assert(cfg != NULL);

  if (reset)
    cfg->statp = cfg->regexps;

  if (cfg->statp) {
    *matches   = cfg->statp->matches;
    *pattern   = cfg->statp->pattern;
    cfg->statp = cfg->statp->next;
    return true;
  }

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
  enum { OVECSIZE = 30 };
  int ovector[OVECSIZE];
  int flags = 0;
  int rc = 0, sc = 0; /* sc = stringcount */

  assert(cfg  != NULL);
  assert(line != NULL);
  assert(buf  != NULL);

  for (r = cfg->regexps; r != NULL; r = r->next) {
    rc = pcre_exec(r->regex, r->data, line, strlen(line), 0, flags, ovector, OVECSIZE);
    if (rc < 0 && rc == PCRE_ERROR_NOMATCH)
      continue;
    if (rc < 0) {
      snprintf(cfg->error, sizeof(cfg->error), "matched failed with error: %d", rc);
      continue;
    }
    /* matched */
    r->matches++;
    sc = (rc) ? rc : OVECSIZE / 3;
    rc = pcre_copy_named_substring(r->regex, line, ovector, sc, "host", buf, buf_size);
    if (rc < 0) {
      snprintf(cfg->error, sizeof(cfg->error), "can't copy matched string: %d", rc);
      continue;
    }
    return true;
  }

  return false;
}

void
destroy(cfg_t *cfg) {
  f2b_regex_t *next = NULL, *r = NULL;

  for (r = cfg->regexps; r != NULL; r = next) {
    next = r->next;
    if (cfg->study)
      pcre_free_study(r->data);
    pcre_free(r->regex);
    free(r);
  }
  free(cfg);
}
