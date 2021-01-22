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
#include <stdarg.h>
#include <string.h>

#include "../strlcpy.h"
#include "../mod-defs.h"
#include "../mod-api.h"

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
 *   f2b <<= filter [label="logcb(level, char *msg)"];
 *   f2b <<  filter [label="false"];
 *       ---        [label="filter is ready for append()"];
 *   f2b =>  filter [label="append(cfg, pattern)"];
 *   f2b <<  filter [label="true"];
 *   f2b =>  filter [label="append(cfg, pattern)"];
 *   f2b <<= filter [label="logcb(level, char *msg)"];
 *   f2b <<  filter [label="false"];
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

#define ID_MAX       32 /**< Maximum length of name for usage in @a start() */
#define PATTERN_MAX 256 /**< Maximum length of regex */
#define HOST_TOKEN "<HOST>" /**< Use this string in place where to search for ip address */

/** opaque pointer to regexp type */
typedef struct _regexp rx_t;

/** type-specific module api routines */

/**
 * @brief Add match pattern
 * @param cfg Module handler
 * @param pattern Regex expression
 * @returns true on success, false on error
 */
extern bool   append(cfg_t *cfg, const char *pattern);
/**
 * @brief Fetch filter match stats
 * @param cfg Module handler
 * @param buf Pointer to storage for stats report
 * @param bufsize Available size for report on @a buf pointer
 * @returns true on success, false on error
 */
extern bool    stats(cfg_t *cfg, char *buf, size_t bufsize);
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
