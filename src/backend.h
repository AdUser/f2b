/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_BACKEND_H_
#define F2B_BACKEND_H_

/**
 * @file
 * This header describes backend module definition and related routines
 */

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
  void  (*logcb)   (void *cfg, void (*cb)(log_msgtype_t lvl, const char *msg));
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
  /* config variables */
  char name[CONFIG_KEY_MAX];  /**< backend name from config (eg [backend:$NAME] section) */
  char init[CONFIG_VAL_MAX];  /**< backend init string (eg `backend = NAME:$INIT_STRING` line from jail section) */
} f2b_backend_t;

/**
 * @brief Allocate new backend struct and fill name/init fields
 * @param name   Module name
 * @param init   Module init string
 * @returns Pointer to allocated module struct or NULL on error
 */
f2b_backend_t * f2b_backend_create (const char *name, const char *init);

/**
 * @brief Initialize and configure backend
 * @param config Pointer to config section with module description
 * @return true on success, false on error
 */
bool f2b_backend_init(f2b_backend_t *backend, f2b_config_section_t *config);

/**
 * @brief Free module metadata
 * @param b Pointer to module struct
 */
void f2b_backend_destroy(f2b_backend_t *b);

/* helpers */
/**
 * @brief Start given backend
 * @param b Pointer to backend struct
 * @returns true on success, false on error with setting last error
 */
bool f2b_backend_start (f2b_backend_t *b);
/**
 * @brief Stop given backend
 * @param b Pointer to backend struct
 * @returns true on success, false on error with setting last error
 */
bool f2b_backend_stop  (f2b_backend_t *b);
/**
 * @brief Perform maintenance of given backend
 * @param b Pointer to backend struct
 * @returns true on success, false on error with setting last error
 */
bool f2b_backend_ping  (f2b_backend_t *b);

/**
 * @brief Send command to ban given ip
 * @param b  Pointer to backend struct
 * @param ip IP address
 * @returns true on success, false on error with setting last error
 */
bool f2b_backend_ban   (f2b_backend_t *b, const char *ip);
/**
 * @brief Check is given ip already banned by backend
 * @param b  Pointer to backend struct
 * @param ip IP address
 * @returns true on success, false on error with setting last error
 * @note Not all backends support this command
 */
bool f2b_backend_check (f2b_backend_t *b, const char *ip);
/**
 * @brief Send command to release given ip
 * @param b  Pointer to backend struct
 * @param ip IP address
 * @returns true on success, false on error with setting last error
 */
bool f2b_backend_unban (f2b_backend_t *b, const char *ip);

#endif /* F2B_BACKEND_H_ */
