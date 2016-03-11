#include "common.h"
#include "log.h"

#include "syslog.h"

static log_msgtype_t minlevel = log_info;
static enum { log_stderr = 0, log_file = 1, log_syslog = 2 } dest = log_stderr;
static FILE *logfile = NULL;

static const char *loglevels[] = {
  "debug",
  "info",
  "warn",
  "error",
  "fatal",
};

static inline int
get_facility(log_msgtype_t l) {
  switch (l) {
    case log_debug: return LOG_DEBUG;
    case log_info:  return LOG_INFO;
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
  if (strcmp(level, "warn")  == 0) { minlevel = log_warn;  return; }
  if (strcmp(level, "error") == 0) { minlevel = log_error; return; }
  if (strcmp(level, "fatal") == 0) { minlevel = log_fatal; return; }
}

void f2b_log_setup(const char *target, const char *path) {
  if (strcmp(target, "syslog") == 0) {
    dest = log_syslog;
    openlog("f2b", LOG_CONS, LOG_DAEMON);
    return;
  } else
  if (strcmp(target, "file") == 0 && *path != '\0') {
    dest = log_file;
    if (logfile)
      fclose(logfile);
    if ((logfile = fopen(path, "a")) == NULL)
      dest = log_stderr, logfile = stderr;
    f2b_log_msg(log_error, "can't open logfile: %s -- %s", path, strerror(errno));
    return;
  } else {
    dest = log_stderr;
    logfile = stderr;
  }

  return;
}
