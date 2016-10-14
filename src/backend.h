/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_BACKEND_H_
#define F2B_BACKEND_H_

#include "config.h"
#include "log.h"

/** backend module definition */
typedef struct f2b_backend_t {
  void *h;   /**< dlopen handler */
  void *cfg; /**< opaque pointer of module config */
  /* handlers */
  /** dlsym pointer to handler of @a create command */
  void *(*create)  (const char *id);
  /** dlsym pointer to handler of @a config command */
  bool  (*config)  (void *cfg, const char *key, const char *value);
  /** dlsym pointer to handler of @a ready command */
  bool  (*ready)   (void *cfg);
  /** dlsym pointer to handler of @a error command */
  char *(*error)   (void *cfg);
  /** dlsym pointer to handler of @a start command */
  bool  (*start)   (void *cfg);
  /** dlsym pointer to handler of @a stop command */
  bool  (*stop)    (void *cfg);
  /** dlsym pointer to handler of @a ping command */
  bool  (*ping)    (void *cfg);
  /** dlsym pointer to handler of @a ban command */
  bool  (*ban)     (void *cfg, const char *ip);
  /** dlsym pointer to handler of @a check command */
  bool  (*check)   (void *cfg, const char *ip);
  /** dlsym pointer to handler of @a unban command */
  bool  (*unban)   (void *cfg, const char *ip);
  /** dlsym pointer to handler of @a destroy command */
  void  (*destroy) (void *cfg);
} f2b_backend_t;

/**
 * @brief Create module from config
 * @param config Pointer to section of config
 * @param init   Module init string
 * @returns Pointer to module metadata of NULL on error
 */
f2b_backend_t * f2b_backend_create (f2b_config_section_t *config, const char *id);
/**
 * @brief Free module metadata
 * @param b Pointer to module struct
 */
void f2b_backend_destroy(f2b_backend_t *b);

/* helpers */
const char *
     f2b_backend_error (f2b_backend_t *b);
bool f2b_backend_start (f2b_backend_t *b);
bool f2b_backend_stop  (f2b_backend_t *b);
bool f2b_backend_ping  (f2b_backend_t *b);

bool f2b_backend_ban   (f2b_backend_t *b, const char *ip);
bool f2b_backend_check (f2b_backend_t *b, const char *ip);
bool f2b_backend_unban (f2b_backend_t *b, const char *ip);

#endif /* F2B_BACKEND_H_ */
