/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../strlcpy.h"

#define INIT_MAX 256

typedef struct _config cfg_t;

extern cfg_t *create(const char *init);
extern bool   config(cfg_t *c, const char *key, const char *value);
extern bool    ready(cfg_t *c);
extern char   *error(cfg_t *c);
extern bool    start(cfg_t *c);
extern bool     stop(cfg_t *c);
extern bool     next(cfg_t *c, char *buf, size_t bufsize, bool reset);
extern void  destroy(cfg_t *c);
