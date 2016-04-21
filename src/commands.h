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

void f2b_cmd_help();
enum f2b_cmsg_type
f2b_cmd_parse(const char *src, char *buf, size_t buflen);

#endif /* F2B_COMMANDS_H_ */
