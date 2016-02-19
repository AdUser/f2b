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

bool f2b_logfile_open(f2b_logfile_t *file, const char *filename);
void f2b_logfile_close(const f2b_logfile_t *file);
bool f2b_logfile_rotated(const f2b_logfile_t *file);
bool f2b_logfile_getline(const f2b_logfile_t *file, char *buf, size_t bufsize);

#endif /* F2B_LOGFILE_H_ */
