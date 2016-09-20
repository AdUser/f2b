/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#if defined(__linux__)
#include <alloca.h>
#endif
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../strlcpy.h"

#define ID_MAX 32
#define PATTERN_MAX 256
#define HOST_TOKEN "<HOST>"

typedef struct _config cfg_t;

extern cfg_t *create(const char *id);
extern const char *error(cfg_t *c);
extern bool   config(cfg_t *c, const char *key, const char *value);
extern bool   append(cfg_t *c, const char *pattern);
extern bool    ready(cfg_t *c);
extern bool    stats(cfg_t *c, int *matches, char **pattern, bool reset);
extern bool    match(cfg_t *c, const char *line, char *buf, size_t bufsize);
extern void    flush(cfg_t *c);
extern void  destroy(cfg_t *c);
