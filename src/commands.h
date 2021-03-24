/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_COMMANDS_H_
#define F2B_COMMANDS_H_

/**
 * @file
 * This header contains definition of control commands and routines
 * for parsing user input
 */

#define CMD_TOKENS_MAXCOUNT 6 /**< Maximum count of data pieces in control message data buf */
#define CMD_TOKENS_MAXSIZE 260 /**< parsed tokens */

enum f2b_command_type {
  CMD_UNKNOWN = 0,  /**< unset */
  CMD_AUTH,         /**< authorization */
  CMD_HELP,         /**< show help for commands */
  CMD_STATUS,       /**< show general status of f2b daemon */
  CMD_RELOAD,       /**< reload all jails */
  CMD_SHUTDOWN,     /**< gracefull shutdown daemon */
  /* logging */
  CMD_LOG_ROTATE,   /**< reopen logfile. (only for `logdest = file`) */
  CMD_LOG_LEVEL,    /**< change maximum level of logged messages */
  /* jail commands */
  CMD_JAIL_STATUS,         /**< show status of given jail */
  CMD_JAIL_SET,            /**< set parameter of given jail */
  CMD_JAIL_IP_STATUS,      /**< show status of given ip */
  CMD_JAIL_IP_BAN,         /**< force ban given ip */
  CMD_JAIL_IP_RELEASE,     /**< force unban given ip */
  CMD_JAIL_SOURCE_STATS,   /**< show stats of source */
  CMD_JAIL_FILTER_STATS,   /**< show stats of filter matches */
  CMD_JAIL_FILTER_RELOAD,  /**< reload filter patterns from file */
  CMD_MAX_NUMBER,          /**< placeholder */
};

/** control command type */
typedef struct f2b_cmd_t {
  enum f2b_command_type type;
  int argc;
  char *args[CMD_TOKENS_MAXCOUNT+1];
  f2b_buf_t data;
} f2b_cmd_t;

/**
 * @brief Returns multiline string with commands list and it's brief descriptions
 * @returns constant multiline string
 */
const char *
f2b_cmd_help();

/**
 * @brief Creates new struct from user input
 * @param line User input string
 * @returns pointer to newly allocated struct or NULL on malloc()/parse error
 */
f2b_cmd_t *
f2b_cmd_create(const char *line);

/**
 * @brief Frees memory
 */
void
f2b_cmd_destroy(f2b_cmd_t *cmd);

/**
 * @brief Try to parse user input
 * @param cmd pointer to preallocated f2b_cmd_t struct
 * @param src Line with user input
 * @returns true on success, false otherwise
 */
bool
f2b_cmd_parse(f2b_cmd_t *cmd, const char *src);

#endif /* F2B_COMMANDS_H_ */
