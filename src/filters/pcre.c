/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "filter.h"

#include <pcre.h>

#define MODNAME "pcre"
#define HOST_REGEX "(?<host>[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})"

struct _regexp {
  rx_t *next;
  pcre *regex;
  pcre_extra *data;
  int matches;
  uint32_t ftag;
  short int score;
  char pattern[PATTERN_MAX];
};

struct _config {
  void (*logcb)(enum loglevel lvl, const char *msg);
  rx_t *regexps;
  int flags;
  short int defscore;
  bool icase;
  bool study;
  bool usejit;
  char id[ID_MAX];
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
  cfg->defscore = MATCH_DEFSCORE;
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
#ifndef PCRE_CONFIG_JIT
    if (cfg->usejit) {
      cfg->usejit = false;
      log_msg(cfg, error, "seems like your pcre library doesn't support jit");
      return false;
    }
#endif
    return true;
  }
  if (strcmp(key, "defscore") == 0) {
    cfg->defscore = atoi(value);
    return true;
  }

  return false;
}

bool
append(cfg_t *cfg, const char *pattern) {
  rx_t *regex = NULL;
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
  strlcat(buf, HOST_REGEX, bufsize);
  strlcat(buf, token + strlen(HOST_TOKEN), bufsize);

  if ((regex = calloc(1, sizeof(rx_t))) == NULL)
    return false;

  if ((regex->regex = pcre_compile(buf, flags, &errptr, &erroffset, NULL)) == NULL) {
    log_msg(cfg, error, "regex compilation failed at %d: %s", erroffset, errptr);
    free(regex);
    return false;
  }

  if (cfg->study) {
    flags = 0;
#ifdef PCRE_CONFIG_JIT
    if (cfg->usejit)
      flags |= PCRE_STUDY_JIT_COMPILE;
#endif
    if ((regex->data = pcre_study(regex->regex, 0, &errptr)) == NULL) {
      log_msg(cfg, error, "regex learn failed: %s", errptr);
      pcre_free(regex->regex);
      free(regex);
      return false;
    }
  }

  regex->score = cfg->defscore;
  regex->ftag = fnv_32a_str(pattern, FNV1_32A_INIT);
  regex->next = cfg->regexps;
  cfg->regexps = regex;
  strlcpy(regex->pattern, pattern, sizeof(regex->pattern));
  cfg->flags |= MOD_IS_READY;
  return true;
}

bool
match(cfg_t *cfg, const char *line, char *buf, size_t buf_size) {
  enum { OVECSIZE = 30 };
  int ovector[OVECSIZE];
  int flags = 0;
  int rc = 0, sc = 0; /* sc = stringcount */

  assert(cfg  != NULL);
  assert(line != NULL);
  assert(buf  != NULL);

  for (rx_t *r = cfg->regexps; r != NULL; r = r->next) {
    rc = pcre_exec(r->regex, r->data, line, strlen(line), 0, flags, ovector, OVECSIZE);
    if (rc < 0 && rc == PCRE_ERROR_NOMATCH)
      continue;
    if (rc < 0) {
      log_msg(cfg, error, "matched failed with error: %d", rc);
      continue;
    }
    /* matched */
    r->matches++;
    sc = (rc) ? rc : OVECSIZE / 3;
    rc = pcre_copy_named_substring(r->regex, line, ovector, sc, "host", buf, buf_size);
    if (rc < 0) {
      log_msg(cfg, error, "can't copy matched string: %d", rc);
      continue;
    }
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
#ifdef PCRE_CONFIG_JIT
    if (cfg->study)
      pcre_free_study(r->data);
#else
    if (cfg->study)
      pcre_free(r->data);
#endif
    pcre_free(r->regex);
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
