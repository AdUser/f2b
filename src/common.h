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
#include <errno.h>
#include <limits.h> /* PATH_MAX */
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/**
 * @file
 * This header contains common includes, usefull macro defs and default paths
 */

#include "strlcpy.h"

/**
 * @def DEFAULT_PIDFILE_PATH
 * Self-descriptive
 */
#define DEFAULT_PIDFILE_PATH "/var/run/f2b.pid"

/**
 * Default path of directory to store ip states for jails
 */
#define DEFAULT_STATEDIR_PATH "/var/db/f2b"

/**
 * @def UNUSED
 * Supresses compiler warning about unused variable
 */
#define UNUSED(x)  (void)(x)

/* default size of buffers */
#define RBUF_SIZE   256
#define WBUF_SIZE 32768 /* 32Kb */

#endif /* F2B_COMMON_H_ */
