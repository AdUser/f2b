/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#if defined(__linux__)
#include <alloca.h>
#endif
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../strlcpy.h"

/**
 * @file
 * This header describes module API of type 'filter'
 *
 * Sample workflow of module usage:
 *
 * @msc
 *   f2b, filter;
 *   f2b =>  filter [label="create(id)"];
 *   f2b <<  filter [label="module handler, cfg_t *cfg"];
 *       |||;
 *   f2b =>  filter [label="config(cfg, param, value)"];
 *   f2b <<  filter [label="true"];
 *   f2b =>  filter [label="config(cfg, param, value)"];
 *   f2b <<  filter [label="true"];
 *   f2b =>  filter [label="config(cfg, param, value)"];
 *   f2b <<  filter [label="false"];
 *   f2b =>  filter [label="error(cfg)"];
 *   f2b <<  filter [label="const char *error"];
 *       ---        [label="filter is ready for append()"];
 *   f2b =>  filter [label="append(cfg, pattern)"];
 *   f2b <<  filter [label="true"];
 *   f2b =>  filter [label="append(cfg, pattern)"];
 *   f2b <<  filter [label="false"];
 *   f2b =>  filter [label="error(cfg)"];
 *   f2b <<  filter [label="const char *error"];
 *       |||        [label="more calls of append()"];
 *   f2b =>  filter [label="ready(cfg)"];
 *   f2b <<  filter [label="true"];
 *       ---        [label="filter is ready to use"];
 *   f2b =>  filter [label="match(cfg, line, buf, sizeof(buf))"];
 *   f2b <<  filter [label="false"];
 *       ...        [label="no match"];
 *   f2b =>  filter [label="match(cfg, line, buf, sizeof(buf))"];
 *   f2b <<  filter [label="true"];
 *       ...        [label="match found, buf filled with ipaddr"];
 *   f2b >>  filter [label="stats(cfg, &matches, &pattern, true)"];
 *   f2b <<  filter [label="true"];
 *   f2b >>  filter [label="stats(cfg, &matches, &pattern, false)"];
 *   f2b <<  filter [label="true"];
 *   f2b >>  filter [label="stats(cfg, &matches, &pattern, false)"];
 *   f2b <<  filter [label="false"];
 *       ...        [label="no more stats"];
 *   f2b =>  filter [label="flush(cfg)"];
 *   f2b <<  filter [label="true"];
 *       ---        [label="now you may config(), append() or destroy() filter"];
 *   f2b =>  filter [label="destroy(cfg)"];
 * @endmsc
 */

/**
 * @def ID_MAX
 * Maximum length of name for usage in @a start()
 */
#define ID_MAX 32
/**
 * @def PATTERN_MAX
 * Maximum length of regex
 */
#define PATTERN_MAX 256
/**
 * @def HOST_TOKEN
 * Use this string in place where to search for ip address
 */
#define HOST_TOKEN "<HOST>"

/**
 * Opaque module handler, contains module internal structs
 */
typedef struct _config cfg_t;

/**
 * @brief Create instance of module
 * @param id Module name string
 * @returns Opaque module handler or NULL on failure
 */
extern cfg_t *create(const char *id);
/**
 * @brief Returns last error description
 * @param cfg Module handler
 * @returns Pointer to string with description of last error
 * @note Returned pointer not marked with const, because libdl complains,
 *       but contents on pointer should not be modified or written in any way
 */
extern const char *error(cfg_t *cfg);
/**
 * @brief Contigure module instance
 * @param cfg Module handler
 * @param key Parameter name
 * @param value Parameter value
 * @returns true on success, false on error with setting intenal error buffer
 */
extern bool   config(cfg_t *cfg, const char *key, const char *value);
/**
 * @brief Add match pattern
 * @param cfg Module handler
 * @param pattern Regex expression
 * @returns true on success, false on error with setting intenal error buffer
 */
extern bool   append(cfg_t *cfg, const char *pattern);
/**
 * @brief Checks is module ready for usage
 * @param cfg Module handler
 * @returns true if ready, false if not
 */
extern bool    ready(cfg_t *cfg);
/**
 * @brief Fetch match stats by-pattern
 * @param cfg Module handler
 * @param matches Pointer to storage for matches count
 * @param pattern Associated pattern
 * @param reset Reset to start of statistics (for later calls)
 * @returns false on no more stats or true otherwise with filling @a matches and @a pattern
 */
extern bool    stats(cfg_t *cfg, int *matches, char **pattern, bool reset);
/**
 * @brief Match given line against configured regexps
 * @param cfg Module handler
 * @param line Source line of data
 * @param buf Buffer for storing match result
 * @param bufsize Size of buffer above
 * @returns false if no match or true otherwise with filling @a buf
 */
extern bool    match(cfg_t *cfg, const char *line, char *buf, size_t bufsize);
/**
 * @brief Destroy all added patterns and free it's resources
 * @param cfg Module handler
 */
extern void    flush(cfg_t *cfg);
/**
 * @brief Free module handle
 * @param cfg Module handler
 * @note Module handler becomes invalid after calling this function on it
 */
extern void  destroy(cfg_t *cfg);
