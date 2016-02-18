#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "log.h"

#define LOGLINE_MAX 1024

void log_msg(log_msgtype_t l, const char *fmt, ...) {
  va_list args;
  char line[LOGLINE_MAX] = "";
  char msg[LOGLINE_MAX]  = "";

  va_start(args, fmt);
  snprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  strncat(line, msg, sizeof(line));

  return;
}
