/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_FILTER_H_
#define F2B_FILTER_H_

/**
 * @file
 * This header describes filter module definition and related routines
 */

/** filter module definition */
typedef struct f2b_filter_t {
  void *h;   /**< dlopen handler */
  void *cfg; /**< opaque pointer of module config */
  int flags; /**< module flags (update with state() call) */
  /* handlers */
  /** dlsym pointer to handler of @a create command */
  void *(*create)  (const char *id);
  /** dlsym pointer to handler of @a config command */
  bool  (*config)  (void *cfg, const char *key, const char *value);
  /** dlsym pointer to handler of @a append command */
  bool  (*append)  (void *cfg, const char *pattern);
  /** dlsym pointer to handler of @a logcb command */
  void  (*logcb)   (void *cfg, void (*cb)(log_msgtype_t lvl, const char *msg));
  /** dlsym pointer to handler of @a state command */
  int   (*state)   (void *cfg);
  /** dlsym pointer to handler of @a flush command */
  bool  (*flush)   (void *cfg);
  /** dlsym pointer to handler of @a stats command */
  bool  (*stats)   (void *cfg, char *buf, size_t bufsize);
  /** dlsym pointer to handler of @a match command */
  uint32_t (*match)(void *cfg, const char *line, char *buf, size_t bufsize, short int *score);
  /** dlsym pointer to handler of @a destroy command */
  void  (*destroy) (void *cfg);
  /* config variables */
  char name[CONFIG_KEY_MAX];  /**< filter name from config (eg [filter:$NAME] section) */
  char init[CONFIG_VAL_MAX];  /**< filter init string (eg `filter = NAME:$INIT_STRING` line from jail section) */
} f2b_filter_t;

/**
 * @brief Allocate new filter struct and fill name/init fields
 * @param name   Module name
 * @param init   Module init string
 * @returns Pointer to allocated module struct or NULL on error
 */
f2b_filter_t * f2b_filter_create  (const char *name, const char *init);

/**
 * @brief Initialize and configure filter
 * @param config Pointer to config section with module description
 * @return true on success, false on error
 */
bool f2b_filter_init (f2b_filter_t *filter, f2b_config_section_t *config);

/**
 * @brief Free module metadata
 * @param f Pointer to module struct
 */
void f2b_filter_destroy (f2b_filter_t *f);

/**
 * @brief Append pattern to filter
 * @param f Pointer to filter struct
 * @param pattern Match pattern
 * @returns true on success, false on error
 */

bool f2b_filter_append(f2b_filter_t *f, const char *pattern);
/**
 * @brief Match a line against given filter
 * @param f Pointer to filter struct
 * @param line Line of data
 * @param buf Match buffer
 * @param bufsize Size of match buffer
 * @param score Pointer to score
 * @returns >0 on match 0 otherwise, fills @a buf with extracted host string and @a score with match score
 */
uint32_t f2b_filter_match (f2b_filter_t *f, const char *line, char *buf, size_t bufsize, short int *score);

/* handlers for csocket commands processing */
/** handler of 'jail $JAIL filter reload' cmd */
void f2b_filter_cmd_reload(char *buf, size_t bufsize, f2b_filter_t *f);

/** handler of 'jail $JAIL filter stats' cmd */
void f2b_filter_cmd_stats (char *buf, size_t bufsize, f2b_filter_t *f);

#endif /* F2B_FILTER_H_ */
