/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_COMMON_H_
#define F2B_COMMON_H_

#include <assert.h>
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "strlcpy.h"

#define DEFAULT_PIDFILE_PATH "/var/run/f2b.pid"
#define DEFAULT_CSOCKET_PATH "/var/run/f2b.sock"
#define DEFAULT_CSOCKET_CPATH "/tmp/f2bc-sock-XXXXXX"

#define UNUSED(x)  (void)(x)

#define SA_REGISTER(SIGNUM, HANDLER) \
  memset(&act, 0x0, sizeof(act)); \
  act.sa_handler = HANDLER; \
  if (sigaction(SIGNUM, &act, NULL) != 0) { \
    f2b_log_msg(log_error, "can't register handler for " #SIGNUM); \
    return EXIT_FAILURE; \
  }

#endif /* F2B_COMMON_H_ */
