/** @file This file contains common exportable types and routines used by all modules */

enum loglevel {
  debug  = 0,
  info   = 1,
  notice = 2,
  warn   = 3,
  error  = 4,
  fatal  = 5,
};

/**
 * Opaque module handler, contains module internal structs
 */
typedef struct _config cfg_t;

/**
 * @brief Create instance of module
 * @param init Module-specific init string
 * @returns Opaque module handler or NULL on failure
 */
extern cfg_t *create(const char *init);

/**
 * @brief Configure module instance
 * @param cfg Module handler
 * @param key Parameter name
 * @param value Parameter value
 * @returns true on success, false on error
 */
extern bool   config(cfg_t *cfg, const char *key, const char *value);

/**
 * @brief Get internal module state as set of flags (see mod.h)
 * @param cfg Module handler
 * @returns <0 on error
 */
extern int    state(cfg_t *cfg);

/**
 * @brief Sets the log callback
 * @param cfg Module handler
 * @param cb Logging callback
 * @note Optional, if this function is not called, warnings/errors of module will be suppressed
 */
extern void    logcb(cfg_t *cfg, void (*cb)(enum loglevel l, const char *msg));

/**
 * @brief Free module handle
 * @param cfg Module handler
 * @note Module handler becomes invalid after calling this function on it
 */
extern void  destroy(cfg_t *cfg);
