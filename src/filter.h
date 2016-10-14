/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_FILTER_H_
#define F2B_FILTER_H_

#include "config.h"
#include "log.h"

/** filter module definition */
typedef struct f2b_filter_t {
  void *h;   /**< dlopen handler */
  void *cfg; /**< opaque pointer of module config */
  char file[PATH_MAX]; /**< path to file with patterns */
  /* handlers */
  /** dlsym pointer to handler of @a create command */
  void *(*create)  (const char *id);
  /** dlsym pointer to handler of @a config command */
  bool  (*config)  (void *cfg, const char *key, const char *value);
  /** dlsym pointer to handler of @a append command */
  bool  (*append)  (void *cfg, const char *pattern);
  /** dlsym pointer to handler of @a error command */
  char *(*error)   (void *cfg);
  /** dlsym pointer to handler of @a ready command */
  bool  (*ready)   (void *cfg);
  /** dlsym pointer to handler of @a flush command */
  bool  (*flush)   (void *cfg);
  /** dlsym pointer to handler of @a stats command */
  bool  (*stats)   (void *cfg, int *matches, char **pattern, bool reset);
  /** dlsym pointer to handler of @a match command */
  bool  (*match)   (void *cfg, const char *line, char *buf, size_t buf_size);
  /** dlsym pointer to handler of @a destroy command */
  void  (*destroy) (void *cfg);
} f2b_filter_t;

f2b_filter_t * f2b_filter_create  (f2b_config_section_t *config, const char *id);
const char *   f2b_filter_error   (f2b_filter_t *f);
bool           f2b_filter_append  (f2b_filter_t *f, const char *pattern);
bool           f2b_filter_match   (f2b_filter_t *f, const char *line, char *buf, size_t buf_size);
void           f2b_filter_destroy (f2b_filter_t *f);

void f2b_filter_cmd_reload(char *buf, size_t bufsize, f2b_filter_t *f);
void f2b_filter_cmd_stats (char *buf, size_t bufsize, f2b_filter_t *f);
#endif /* F2B_FILTER_H_ */
