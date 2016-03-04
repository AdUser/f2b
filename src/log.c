#include "common.h"
#include "log.h"

static log_msgtype_t minlevel = log_info;

static const char *loglevels[] = {
  "debug",
  "info",
  "warn",
  "error",
  "fatal",
};

void f2b_log_msg(log_msgtype_t l, const char *fmt, ...) {
  va_list args;
  char line[LOGLINE_MAX] = "";
  char msg[LOGLINE_MAX]  = "";

  if (l < minlevel)
    return;

  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  snprintf(line, LOGLINE_MAX, "[%s] %s", loglevels[l], msg);
  /* TODO */
  fputs(line, stderr);
  fputc('\n', stderr);

  return;
}

void f2b_log_set_level(const char *level) {
  if (strcmp(level, "debug") == 0) { minlevel = log_debug; return; }
  if (strcmp(level, "info")  == 0) { minlevel = log_info;  return; }
  if (strcmp(level, "warn")  == 0) { minlevel = log_warn;  return; }
  if (strcmp(level, "error") == 0) { minlevel = log_error; return; }
  if (strcmp(level, "fatal") == 0) { minlevel = log_fatal; return; }
}
