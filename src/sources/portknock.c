/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "source.h"

struct _config {
  char error[256];
  void (*errcb)(char *errstr);
};

static void
errcb_stub(char *str) {
  assert(str != NULL);
  (void)(str);
}

cfg_t *
create(const char *init) {
  cfg_t *cfg = NULL;
  assert(init != NULL);
  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  strlcpy(cfg->path, init, sizeof(cfg->path));
  cfg->errcb = &errcb_stub;
  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key   != NULL);
  assert(value != NULL);
  /* no options */
  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);
  if (cfg->path[0] == '\0')
    return false;
  return true;
}

char *
error(cfg_t *cfg) {
  assert(cfg != NULL);

  return cfg->error;
}

void
errcb(cfg_t *cfg, void (*cb)(char *errstr)) {
  assert(cfg != NULL);
  assert(cb  != NULL);

  cfg->errcb = cb;
}

bool
start(cfg_t *cfg) {
  assert(cfg != NULL);

  /* TODO */
  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  /* TODO */
  return true;
}

bool
next(cfg_t *cfg, char *buf, size_t bufsize, bool reset) {
  assert(cfg != NULL);
  assert(buf != NULL);
  assert(bufsize > 0);

  /* TODO */
  return false;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
