/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_CONFIG_H_
#define F2B_CONFIG_H_

#define CONFIG_LINE_MAX 256

#define CONFIG_KEY_MAX 32
#define CONFIG_VAL_MAX 192

/** Section types in config */
typedef enum f2b_section_type {
  t_unknown = 0,
  t_main,
  t_defaults,
  t_source,
  t_filter,
  t_backend,
  t_jail,
} f2b_section_type;

/** Key-value line in config */
typedef struct f2b_config_param_t {
  struct f2b_config_param_t *next; /**< pointer to next parameter of this section */
  char name[CONFIG_KEY_MAX];  /**< parameter name  */
  char value[CONFIG_VAL_MAX]; /**< parameter value */
} f2b_config_param_t;

/** Section of config */
typedef struct f2b_config_section_t {
  struct f2b_config_section_t *next; /**< pointer to next section of same type */
  char name[CONFIG_KEY_MAX]; /**< section name (eg [type:$NAME]) */
  f2b_section_type type;     /**< section type */
  f2b_config_param_t *param; /**< linked list of parameters */
  f2b_config_param_t *last;  /**< tail of parameter list */
} f2b_config_section_t;

/** topmost f2b config struct */
typedef struct f2b_config_t {
  f2b_config_section_t *main;      /**< section [main] */
  f2b_config_section_t *defaults;  /**< section [defaults]   */
  f2b_config_section_t *sources;   /**< sections [source:*]  */
  f2b_config_section_t *filters;   /**< sections [filter:*]  */
  f2b_config_section_t *backends;  /**< sections [backend:*] */
  f2b_config_section_t *jails;     /**< sections [jail:*]    */
} f2b_config_t;

/**
 * @brief Try parse line with `key = value` parameter
 * @param src Source line
 * @returns Pointer to created allocated parameter struct or NULL on error
 */
f2b_config_param_t * f2b_config_param_create(const char *src);

/**
 * @brief Find parameter with given name in list
 * @param list Linked list of parameters
 * @param name Name of wanted parameter
 * @returns Pointer to found parameter or NULL if nothing found
 */
f2b_config_param_t * f2b_config_param_find (f2b_config_param_t *list, const char *name);

/**
 * @brief Find section with given name in list
 * @param list Linked list of sections
 * @param name Name of wanted section
 * @returns Pointer to found section or NULL if nothing found
 */
f2b_config_section_t * f2b_config_section_find (f2b_config_section_t *list, const char *name);

/**
 * @brief Load config from file
 * @param c Config struct pointer
 * @param path Path to config file
 * @param recursion Process `include = <dir>` parameter
 * @returns true on success, false if error(s) occured
 */
bool f2b_config_load(f2b_config_t *c, const char *path, bool recursion);
/**
 * @brief Destroy config and free all resources
 * @param c Config pointer
 */
void f2b_config_free(f2b_config_t *c);

#endif /* F2B_CONFIG_H_ */
