/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdbool.h>

#define TOKEN_ID "<ID>"
#define TOKEN_IP "<IP>"
#define ID_MAX 32

typedef struct _config cfg_t;

extern cfg_t *create(const char *id);
extern bool   config(cfg_t *c, const char *key, const char *value);
extern bool    ready(cfg_t *c);
extern char   *error(cfg_t *c);
extern bool    start(cfg_t *c);
extern bool     stop(cfg_t *c);
extern bool     ping(cfg_t *c);
extern bool      ban(cfg_t *c, const char *ip);
extern bool    check(cfg_t *c, const char *ip);
extern bool    unban(cfg_t *c, const char *ip);
extern void  destroy(cfg_t *c);
