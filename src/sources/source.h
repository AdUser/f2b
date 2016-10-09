/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../strlcpy.h"

/**
 * @file
 * This header describes module API of type 'source'
 *
 * Sample workflow of module usage:
 *
 * @msc
 *   f2b, source;
 *   f2b =>  source [label="create(init)"];
 *   f2b <<  source [label="module handler, cfg_t *cfg"];
 *       |||;
 *   f2b =>  source [label="config(cfg, param, value)"];
 *   f2b <<  source [label="true"];
 *   f2b =>  source [label="config(cfg, param, value)"];
 *   f2b <<  source [label="true"];
 *   f2b =>  source [label="config(cfg, param, value)"];
 *   f2b <<  source [label="false"];
 *   f2b =>  source [label="error(cfg)"];
 *   f2b <<  source [label="const char *error"];
 *       |||;
 *   f2b =>  source [label="errcb(cfg, cb), optional"];
 *       |||;
 *   f2b =>  source [label="ready(cfg)"];
 *   f2b <<  source [label="true"];
 *       ---        [label="source is ready for start()"];
 *   f2b =>  source [label="start()"];
 *   f2b <<  source [label="true"];
 *       ---        [label="source is ready to use"];
 *   f2b =>  source [label="next(cfg, buf, sizeof(buf), true)"];
 *   f2b <<  source [label="false"];
 *       ...        [label="no data this time, try later"];
 *   f2b =>  source [label="next(cfg, buf, sizeof(buf), true)"];
 *   f2b <<  source [label="true"];
 *   f2b =>  source [label="next(cfg, buf, sizeof(buf), false)"];
 *   f2b <<= source [label="errcb(char *error)"];
 *   f2b <<  source [label="true"];
 *   f2b =>  source [label="next(cfg, buf, sizeof(buf), false)"];
 *   f2b <<  source [label="false"];
 *       ...        [label="time passed"];
 *   f2b =>  source [label="stop(cfg)"];
 *   f2b <<  source [label="true"];
 *       ---        [label="now you may config(), start() or destroy() source"];
 *   f2b =>  source [label="destroy(cfg)"];
 * @endmsc
 */

/**
 * @def INIT_MAX
 * Defines max length of @a init param in @a create()
 */
#define INIT_MAX 256

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
 * @returns true on success, false on error with setting intenal error buffer
 */
extern bool   config(cfg_t *cfg, const char *key, const char *value);
/**
 * @brief Checks is module ready for usage
 * @param cfg Module handler
 * @returns true if ready, false if not
 */
extern bool    ready(cfg_t *cfg);
/**
 * @brief Returns last error description
 * @param cfg Module handler
 * @returns Pointer to string with description of last error
 * @note Returned pointer not marked with const, because libdl complains,
 *       but contents on pointer should not be modified or written in any way
 */
extern char   *error(cfg_t *cfg);
/**
 * @brief Sets the error callback for use in processing
 * @param cfg Module handler
 * @param cb Error callback
 * @note Optional, if this function is not called, processing errors will be suppressed
 */
extern void    errcb(cfg_t *cfg, void (*cb)(char *errstr));
/**
 * @brief Allocate resources and start processing
 * @param cfg Module handler
 * @returns true on success, false on error with setting intenal error buffer
 */
extern bool    start(cfg_t *cfg);
/**
 * @brief Poll source for new data
 * @param cfg Module handler
 * @param buf Pointer to buffer for storing result
 * @param bufsize Size of buffer above (in bytes)
 * @param reset Reset internals to start of list
 * @returns false if no new data available, or true otherwise with filling @a buf
 */
extern bool     next(cfg_t *cfg, char *buf, size_t bufsize, bool reset);
/**
 * @brief Deallocate resources, prepare for module destroy
 * @param cfg Module handler
 * @returns true on success
 */
extern bool     stop(cfg_t *cfg);
/**
 * @brief Free module handle
 * @param cfg Module handler
 * @note Module handler becomes invalid after calling this function on it
 */
extern void  destroy(cfg_t *cfg);
