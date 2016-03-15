/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_LOG_H_
#define F2B_LOG_H_

#define LOGLINE_MAX 1024

typedef enum {
  log_debug = 0,
  log_info  = 1,
  log_note  = 2,
  log_warn  = 3,
  log_error = 4,
  log_fatal = 5
} log_msgtype_t;

void f2b_log_msg(log_msgtype_t l, const char *fmt, ...);
void f2b_log_set_level(const char *level);
void f2b_log_to_file  (const char *path);
void f2b_log_to_stderr();
void f2b_log_to_syslog();

#endif /* F2B_LOG_H_ */
