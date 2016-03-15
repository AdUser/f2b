/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "log.h"

#include "syslog.h"

static log_msgtype_t minlevel = log_info;
static enum { log_stderr = 0, log_file = 1, log_syslog = 2 } dest = log_stderr;
static FILE *logfile = NULL;

static const char *loglevels[] = {
  "debug",
  "info",
  "notice",
  "warn",
  "error",
  "fatal",
};

static inline int
get_facility(log_msgtype_t l) {
  switch (l) {
    case log_debug: return LOG_DEBUG;
    case log_info:  return LOG_INFO;
    case log_note:  return LOG_NOTICE;
    case log_warn:  return LOG_WARNING;
    case log_error: return LOG_ERR;
    case log_fatal: return LOG_CRIT;
      break;
  }
  return LOG_INFO;
}

void f2b_log_msg(log_msgtype_t l, const char *fmt, ...) {
  va_list args;
  char msg[LOGLINE_MAX]  = "";

  if (l < minlevel)
    return;

  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  switch (dest) {
    case log_syslog:
      syslog(get_facility(l), "%s", msg);
      break;
    default:
    case log_stderr:
      logfile = stderr;
    case log_file:
      fprintf(logfile, "[%s] %s\n", loglevels[l], msg);
      break;
  }

  return;
}

void f2b_log_set_level(const char *level) {
  if (strcmp(level, "debug") == 0) { minlevel = log_debug; return; }
  if (strcmp(level, "info")  == 0) { minlevel = log_info;  return; }
  if (strcmp(level, "notice")== 0) { minlevel = log_note;  return; }
  if (strcmp(level, "warn")  == 0) { minlevel = log_warn;  return; }
  if (strcmp(level, "error") == 0) { minlevel = log_error; return; }
  if (strcmp(level, "fatal") == 0) { minlevel = log_fatal; return; }
}

void f2b_log_to_stderr() {
  if (logfile && logfile != stderr)
    fclose(logfile);
  dest = log_stderr;
  logfile = stderr;
}

void f2b_log_to_file(const char *path) {
  FILE *new = NULL;
  if (path == NULL || *path == '\0')
    return;
  if ((new = fopen(path, "a")) != NULL) {
    if (logfile && logfile != stderr)
      fclose(logfile);
    dest = log_file;
    logfile = new;
  }
}

void f2b_log_to_syslog() {
  if (logfile && logfile != stderr)
    fclose(logfile);
  dest = log_syslog;
  openlog("f2b", 0 | LOG_PID, LOG_DAEMON);
}
