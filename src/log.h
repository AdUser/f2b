/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_LOG_H_
#define F2B_LOG_H_

/**
 * @file
 * This file contains logging routines
 */

/**
 * @def LOGLINE_MAX
 * Maximum length of log message
 */
#define LOGLINE_MAX 1024

/** levels of log messages */
typedef enum {
  log_debug = 0, /**< diagnostic messages */
  log_info  = 1, /**< usefull, but not important messages */
  log_note  = 2, /**< ban/unban events */
  log_warn  = 3, /**< something goes wrong */
  log_error = 4, /**< error messages */
  log_fatal = 5  /**< critical error, program terminates */
} log_msgtype_t;

/**
 * @brief Write message to log
 * @param l Level of message
 * @param fmt Message format string
 */
void f2b_log_msg(log_msgtype_t l, const char *fmt, ...)
  __attribute__ ((format (printf, 2, 3)));
/**
 * @brief Logging wrapper for use in source module
 * @param errstr Error string
 */
void f2b_log_error_cb(const char *errstr);
/**
 * @brief Limit logging messages by importance
 * @param level Min level of messages for logging
 */
void f2b_log_set_level(const char *level);
/**
 * @brief Use logging to file
 * @param path Path for logfile
 */
void f2b_log_to_file  (const char *path);
/** @brief Use logging to stderr */
void f2b_log_to_stderr();
/** @brief Use logging to syslog */
void f2b_log_to_syslog();

#endif /* F2B_LOG_H_ */
