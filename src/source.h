/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_SOURCE_H_
#define F2B_SOURCE_H_

/**
 * @file
 * This header describes source module definition and related routines
 */

/** source module definition */
typedef struct f2b_source_t {
  void *h;   /**< dlopen handler */
  void *cfg; /**< opaque pointer of module config */
  int flags; /**< module flags (update with state() call) */
  /* handlers */
  /** dlsym pointer to handler of @a create command */
  void *(*create)  (const char *init);
  /** dlsym pointer to handler of @a config command */
  bool  (*config)  (void *cfg, const char *key, const char *value);
  /** dlsym pointer to handler of @a state command */
  int   (*state)   (void *cfg);
  /** dlsym pointer to handler of @a logcb command */
  void  (*logcb)   (void *cfg, void (*cb)(log_msgtype_t l, const char *msg));
  /** dlsym pointer to handler of @a start command */
  bool  (*start)   (void *cfg);
  /** dlsym pointer to handler of @a next command */
  uint32_t (*next) (void *cfg, char *buf, size_t bufsize, bool reset);
  /** dlsym pointer to handler of @a stats command */
  bool  (*stats)   (void *cfg, char *buf, size_t bufsize);
  /** dlsym pointer to handler of @a stop command */
  bool  (*stop)    (void *cfg);
  /** dlsym pointer to handler of @a destroy command */
  void  (*destroy) (void *cfg);
  /* config variables */
  char name[CONFIG_KEY_MAX];  /**< source name from config (eg [source:$NAME] section) */
  char init[CONFIG_VAL_MAX];  /**< source init string (eg `source = NAME:$INIT_STRING` line from jail section) */
} f2b_source_t;

/**
 * @brief Allocate new source struct and fill name/init fields
 * @param name   Module name
 * @param init   Module init string
 * @returns Pointer to allocated module struct or NULL on error
 */
f2b_source_t * f2b_source_create(const char *name, const char *init);

/**
 * @brief Initialize and configure source
 * @param config Pointer to config section with module description
 * @return true on success, false on error
 */
bool f2b_source_init(f2b_source_t *source, f2b_config_section_t *config);

/**
 * @brief Free module metadata
 * @param s Pointer to module struct
 */
void f2b_source_destroy (f2b_source_t *s);

/**
 * @brief Start given source
 * @param s Pointer to source struct
 * @returns true on success, false on error
 */
bool f2b_source_start (f2b_source_t *s);
/**
 * @brief Get next line of data from given source
 * @param s Pointer to source struct
 * @param buf Buffer for data
 * @param bufsize Size of buffer for data
 * @param reset Reset source internals
 * @returns >0 on new data available with filling @a buf and 0 on no data/error
 */
uint32_t f2b_source_next (f2b_source_t *s, char *buf, size_t bufsize, bool reset);
/**
 * @brief Get internal stats from source
 * @param buf Buffer for data
 * @param bufsize Size of buffer for data
 * @returns true on success, false on error
 */
bool f2b_source_stats (f2b_source_t *s, char *buf, size_t bufsize);
/**
 * @brief Stop given source
 * @param s Pointer to source struct
 * @returns true on success, false on error
 */
bool f2b_source_stop (f2b_source_t *s);

/* handlers for csocket commands processing */

/** handler of 'jail $JAIL source stats' cmd */
void f2b_source_cmd_stats (char *buf, size_t bufsize, f2b_source_t *f);

#endif /* F2B_SOURCE_H_ */
