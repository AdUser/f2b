#ifndef F2B_COMMON_H_
#define F2B_COMMON_H_

#include <alloca.h>
#include <assert.h>
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FREE(x)   free(x), x = NULL

#endif /* F2B_COMMON_H_ */
