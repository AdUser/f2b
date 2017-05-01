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
 * for work with data buffer of control message
 */

/**
 * Maximum length of input line in client
 * @note yes, i know about LINE_MAX
 */
#define INPUT_LINE_MAX 256
/** Maximum count of data pieces in control message data buf */
#define CMD_TOKENS_MAX 6

/** control command type */
enum f2b_cmd_type {
  CMD_NONE = 0,  /**< unset */
  CMD_RESP,      /**< response of command */
  CMD_HELP,      /**< show help for commands (used internally by client) */
  CMD_PING = 8,  /**< check connection */
  CMD_STATUS,    /**< show general status of f2b daemon */
  CMD_LOG_ROTATE,/**< reopen logfile. works only if set `logdest = file` */
  CMD_RELOAD,    /**< reload all jails */
  CMD_SHUTDOWN,  /**< gracefull shutdown */
  CMD_LOG_LEVEL, /**< change maximum level of logged messages */
  /* jail commands */
  CMD_JAIL_STATUS = 16,    /**< show status of given jail */
  CMD_JAIL_SET,            /**< set parameter of given jail */
  CMD_JAIL_IP_STATUS,      /**< show status of given ip */
  CMD_JAIL_IP_BAN,         /**< force ban given ip */
  CMD_JAIL_IP_RELEASE,     /**< force unban given ip */
  CMD_JAIL_FILTER_STATS,   /**< show stats of fileter matches */
  CMD_JAIL_FILTER_RELOAD,  /**< reload filter patterns from file */
  CMD_MAX_NUMBER,          /**< placeholder */
};

/**
 * @brief Print to stdout help for defined commands
 */
void f2b_cmd_help();

/**
 * @brief Try to parse user input
 * @param buf Buffer of control message for storing parsed args
 * @param bufsize Size of buffer size above
 * @param src Line with user input
 * @returns @a CMD_NONE if parsing fails, or cmd type less than @a CMD_MAX_NUMBER on success
 */
enum f2b_cmd_type
f2b_cmd_parse(char *buf, size_t bufsize, const char *src);

/**
 * @brief Append data piece to data buffer of control message
 * @param buf Buffer of control message for storing parsed args
 * @param bufsize Size of buffer size above
 * @param arg Piece to append
 */
void
f2b_cmd_append_arg(char *buf, size_t bufsize, const char *arg);
/**
 * @brief Checks is args count match given command type
 * @param type Command type
 * @param argc Args count
 * @returns true if matches, false if not
 */
bool
f2b_cmd_check_argc(enum f2b_cmd_type type, int argc);

#endif /* F2B_COMMANDS_H_ */
