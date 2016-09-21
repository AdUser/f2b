/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_COMMANDS_H_
#define F2B_COMMANDS_H_

/* yes, i know about LINE_MAX */
#define INPUT_LINE_MAX 256
#define CMD_TOKENS_MAX 6

enum f2b_cmd_type {
  CMD_NONE = 0,
  CMD_RESP,
  CMD_HELP,
  CMD_PING = 8,
  CMD_STATUS,
  CMD_ROTATE,
  CMD_RELOAD,
  CMD_SHUTDOWN,
  CMD_JAIL_STATUS = 16,
  CMD_JAIL_SET,
  CMD_JAIL_IP_STATUS,
  CMD_JAIL_IP_BAN,
  CMD_JAIL_IP_RELEASE,
  CMD_JAIL_FILTER_STATS,
  CMD_JAIL_FILTER_RELOAD,
  CMD_MAX_NUMBER,
};

void f2b_cmd_help();
enum f2b_cmd_type
f2b_cmd_parse     (char *buf, size_t bufsize, const char *src);

void
f2b_cmd_append_arg(char *buf, size_t bufsize, const char *arg);

bool
f2b_cmd_check_argc(enum f2b_cmd_type type, int argc);

#endif /* F2B_COMMANDS_H_ */
