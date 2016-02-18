#ifndef F2B_LOGFILE_H_
#define F2B_LOGFILE_H_

#include <limits.h>
#include <sys/stat.h>

typedef struct {
  struct f2b_logfile_t *next;
  char path[PATH_MAX];
  FILE *fd;
  struct stat st;
} f2b_logfile_t;

#endif /* F2B_LOGFILE_H_ */
