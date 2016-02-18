#include <sys/stat.h>

#include "common.h"
#include "log.h"
#include "logfile.h"

bool
f2b_logfile_open(f2b_logfile_t *file, const char *filename) {
  struct stat st;

  assert(file != NULL);
  assert(filename != NULL);

  memset(file, 0x0, sizeof(f2b_logfile_t));

  if (stat(filename, &st) != 0) {
    log_msg(log_error, "can't open file %s: %s", filename, strerror(errno));
    return false;
  }

  strncpy(file->path, filename, sizeof(file->path));
  memcpy(&file->st, &st, sizeof(st));

  if ((file->fd = fopen(filename, "r")) == NULL) {
    log_msg(log_error, "can't open file %s: %s", filename, strerror(errno));
    return false;
  }

  if (fseek(file->fd, 0, SEEK_END) < 0) {
    log_msg(log_error, "can't seek to end of file %s: %s", filename, strerror(errno));
    return false;
  }

  return true;
}

void
f2b_logfile_close(const f2b_logfile_t *file);

bool
f2b_logfile_rotated(const f2b_logfile_t *file);

ssize_t
f2b_logfile_getline(const f2b_logfile_t *file, const char *buf, size_t bufsize) {
  return -1;
}
