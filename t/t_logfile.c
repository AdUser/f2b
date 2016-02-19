#include <stdlib.h>

#include "../src/common.h"
#include "../src/logfile.h"

int main() {
  f2b_logfile_t file;
  char filename[] = "/tmp/f2b-test.XXXXXX";
  int fd = 0;
  FILE *wfd = NULL;
  char buf[2048];

  assert((fd = mkstemp(filename)) > 0);
  assert((wfd = fdopen(fd, "a")) != NULL);
  assert(fputs("test1\n", wfd) > 0);
  assert(fflush(wfd) == 0);

  assert(f2b_logfile_open(&file, filename) == true);
  assert(f2b_logfile_getline(&file, buf, sizeof(buf)) == false);

  assert(fputs("test2\n", wfd) > 0);
  assert(fflush(wfd) == 0);

  assert(f2b_logfile_getline(&file, buf, sizeof(buf)) == true);
  assert(strncmp(buf, "test2\n", sizeof(buf)) == 0);

  assert(f2b_logfile_getline(&file, buf, sizeof(buf)) == false);

  fclose(wfd);
  close(fd);
  unlink(filename);

  assert((wfd = fopen(filename, "a")) != 0);

  assert(f2b_logfile_rotated(&file) == true);

  f2b_logfile_close(&file);
  fclose(wfd);
  unlink(filename);

  exit(0);
}
