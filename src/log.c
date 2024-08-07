/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "log.h"

#include <sys/syslog.h>

static log_msgtype_t minlevel = log_info;
static enum {
  log_stderr = 0,
  log_stdout = 1,
  log_file   = 2,
  log_syslog = 3
} dest = log_stderr;
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
  char when[64] = "";
  time_t now = time(NULL);

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
      /* fallthru */
    case log_file:
      strftime(when, sizeof(when), "%F %H:%M:%S", localtime(&now));
      fprintf(logfile, "%s [%s] %s\n", when, loglevels[l], msg);
      break;
  }

  return;
}

void f2b_log_mod_cb(log_msgtype_t l, const char *msg) {
  f2b_log_msg(l, "%s", msg);
}

void f2b_log_set_level(const char *level) {
  if (strcmp(level, "debug") == 0) { minlevel = log_debug; return; }
  if (strcmp(level, "info")  == 0) { minlevel = log_info;  return; }
  if (strcmp(level, "notice")== 0) { minlevel = log_note;  return; }
  if (strcmp(level, "warn")  == 0) { minlevel = log_warn;  return; }
  if (strcmp(level, "error") == 0) { minlevel = log_error; return; }
  if (strcmp(level, "fatal") == 0) { minlevel = log_fatal; return; }
}

void f2b_log_to_stdout() {
  if (logfile && logfile != stdout)
    fclose(logfile);
  dest = log_stdout;
  logfile = stdout;
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
    setvbuf(new, NULL , _IONBF, 0);
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
