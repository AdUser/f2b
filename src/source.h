/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_SOURCE_H_
#define F2B_SOURCE_H_

#include "config.h"
#include "log.h"

typedef struct f2b_source_t {
  void *h;
  void *cfg;
  void *(*create)  (const char *init);
  bool  (*config)  (void *cfg, const char *key, const char *value);
  bool  (*ready)   (void *cfg);
  char *(*error)   (void *cfg);
  void  (*errcb)   (void *cfg, void (*cb)(const char *errstr));
  bool  (*start)   (void *cfg);
  bool  (*next)    (void *cfg, char *buf, size_t bufsize, bool reset);
  bool  (*stop)    (void *cfg);
  void  (*destroy) (void *cfg);
} f2b_source_t;

f2b_source_t * f2b_source_create  (f2b_config_section_t *config, const char *init, void (*errcb)(const char *));
void           f2b_source_destroy (f2b_source_t *s);
bool           f2b_source_next    (f2b_source_t *s, char *buf, size_t bufsize, bool reset);
const char *   f2b_source_error   (f2b_source_t *s);

#endif /* F2B_SOURCE_H_ */
