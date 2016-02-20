#ifndef F2B_COMMON_H_
#define F2B_COMMON_H_

#include <assert.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define FREE(x)   free(x), x = NULL

#endif /* F2B_COMMON_H_ */
