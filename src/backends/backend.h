/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdbool.h>
#include <stdarg.h>

#include "../mod-defs.h"
#include "../mod-api.h"

/**
 * @file
 * This header describes module API of type 'backend'
 *
 * Sample workflow of module usage:
 *
 * @msc
 *   f2b, backend;
 *   f2b =>  backend [label="create(init)"];
 *   f2b <<  backend [label="module handler, cfg_t *cfg"];
 *       |||;
 *   f2b =>  backend [label="config(cfg, param, value)"];
 *   f2b <<  backend [label="true"];
 *   f2b =>  backend [label="config(cfg, param, value)"];
 *   f2b <<  backend [label="true"];
 *   f2b =>  backend [label="config(cfg, param, value)"];
 *   f2b <<= backend [label="logcb(level, char *msg)"];
 *   f2b <<  backend [label="false"];
 *       |||;
 *   f2b =>  backend [label="ready(cfg)"];
 *   f2b <<  backend [label="true"];
 *       ---         [label="backend is ready for start()"];
 *   f2b =>  backend [label="start()"];
 *   f2b <<  backend [label="true"];
 *       ---         [label="backend is ready to use"];
 *   f2b =>  backend [label="ping(cfg)"];
 *   f2b <<  backend [label="true"];
 *   f2b =>  backend [label="ping(cfg)"];
 *   f2b <<  backend [label="true"];
 *       ...         [label="time passed"];
 *   f2b =>  backend [label="ban(cfg, ip)"];
 *   f2b <<  backend [label="true"];
 *   f2b =>  backend [label="ban(cfg, ip)"];
 *   f2b <<= backend [label="logcb(level, char *msg)"];
 *   f2b <<  backend [label="false"];
 *       ...         [label="time passed"];
 *   f2b =>  backend [label="ping(cfg)"];
 *   f2b <<  backend [label="true"];
 *       ...         [label="time passed"];
 *   f2b =>  backend [label="unban(cfg, ip)"];
 *   f2b <<  backend [label="true"];
 *       ...         [label="time passed"];
 *   f2b =>  backend [label="stop(cfg)"];
 *   f2b <<  backend [label="true"];
 *       ---         [label="now you may config(), start() or destroy() backend"];
 *   f2b =>  backend [label="destroy(cfg)"];
 * @endmsc
 */

/**
 * @def TOKEN_ID
 * Only 'exec' backend: placeholder for module id, in ban(), check() and unban() actions
 */
#define TOKEN_ID "<ID>"
/**
 * @def TOKEN_IP
 * Only 'exec' backend: placeholder for ip in ban(), check() and unban() actions
 */
#define TOKEN_IP "<IP>"
/**
 * @def ID_MAX
 * maximum lenth of id from @a create()
 */
#define ID_MAX 32

/** @note config() parameters are module-specific, but each module
 * at least should handle 'shared' option */

/** type-specific module exportable routines */

/**
 * @brief Allocate resources and start processing
 * @param cfg Module handler
 * @returns true on success, false on error
 */
extern bool    start(cfg_t *cfg);
/**
 * @brief Deallocate resources, prepare for module destroy
 * @param cfg Module handler
 * @returns true on success
 */
extern bool     stop(cfg_t *cfg);
/**
 * @brief Module maintenance routine
 * @param cfg Module handler
 * @returns true on success
 */
extern bool     ping(cfg_t *cfg);
/**
 * @brief Make a rabbit disappear
 * @param cfg Module handler
 * @param ip IP address
 * @returns true on success, false on error
 */
extern bool      ban(cfg_t *cfg, const char *ip);
/**
 * @brief Check is some ip already banned
 * @param cfg Module handler
 * @param ip IP address
 * @returns true on success, false on error
 * @note If this action is meaningless for backend it should return true
 */
extern bool    check(cfg_t *cfg, const char *ip);
/**
 * @brief Make a rabbit appear again
 * @param cfg Module handler
 * @param ip IP address
 * @returns true on success, false on error
 * @note If this action is meaningless for backend it should return true
 */
extern bool    unban(cfg_t *cfg, const char *ip);
