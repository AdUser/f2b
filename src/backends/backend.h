/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdbool.h>

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
 *   f2b <<  backend [label="false"];
 *   f2b =>  backend [label="error(cfg)"];
 *   f2b <<  backend [label="const char *error"];
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
 *   f2b <<  backend [label="false"];
 *   f2b =>  backend [label="error(cfg)"];
 *   f2b <<  backend [label="const char *error"];
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

/**
 * Opaque module handler, contains module internal structs
 */
typedef struct _config cfg_t;

/**
 * @brief Create instance of module
 * @param id Module id, used when 'shared = yes'
 * @returns Opaque module handler or NULL on failure
 */
extern cfg_t *create(const char *id);
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
 * @note parameters are module-specific, but each module
 * at least should handle 'shared' option
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
 * @brief Allocate resources and start processing
 * @param cfg Module handler
 * @returns true on success, false on error with setting intenal error buffer
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
 * @returns true on success, false on error with setting intenal error buffer
 */
extern bool      ban(cfg_t *cfg, const char *ip);
/**
 * @brief Check is some ip already banned
 * @param cfg Module handler
 * @param ip IP address
 * @returns true on success, false on error with setting intenal error buffer
 * @note If this action is meaningless for backend it should return true
 */
extern bool    check(cfg_t *cfg, const char *ip);
/**
 * @brief Make a rabbit appear again
 * @param cfg Module handler
 * @param ip IP address
 * @returns true on success, false on error with setting intenal error buffer
 * @note If this action is meaningless for backend it should return true
 */
extern bool    unban(cfg_t *cfg, const char *ip);
/**
 * @brief Free module handle
 * @param cfg Module handler
 * @note Module handler becomes invalid after calling this function on it
 */
extern void  destroy(cfg_t *cfg);
