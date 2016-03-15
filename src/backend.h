/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef BACKEND_H_
#define BACKEND_H_

#include "config.h"
#include "log.h"

typedef struct f2b_backend_t {
  void *h;
  void *cfg;
  void *(*create)  (const char *id);
  bool  (*config)  (void *cfg, const char *key, const char *value);
  bool  (*ready)   (void *cfg);
  char *(*error)   (void *cfg);
  bool  (*start)   (void *cfg);
  bool  (*stop)    (void *cfg);
  bool  (*ping)    (void *cfg);
  bool  (*ban)     (void *cfg, const char *ip);
  bool  (*check)   (void *cfg, const char *ip);
  bool  (*unban)   (void *cfg, const char *ip);
  void  (*destroy) (void *cfg);
} f2b_backend_t;

f2b_backend_t * f2b_backend_create (f2b_config_section_t *config, const char *id);
void            f2b_backend_destroy(f2b_backend_t *b);

const char *
     f2b_backend_error (f2b_backend_t *b);
bool f2b_backend_start (f2b_backend_t *b);
bool f2b_backend_stop  (f2b_backend_t *b);
bool f2b_backend_ping  (f2b_backend_t *b);

bool f2b_backend_ban   (f2b_backend_t *b, const char *ip);
bool f2b_backend_check (f2b_backend_t *b, const char *ip);
bool f2b_backend_unban (f2b_backend_t *b, const char *ip);

#endif /* BACKEND_H_ */
