#include "../src/common.h"
#include "../src/logfile.h"

int main() {
  f2b_logfile_t file;
  char filename[] = "/tmp/f2b-test.XXXXXX";
  int fd = 0;
  int ret;
  bool res;
  FILE *wfd = NULL;
  char buf[2048];

  UNUSED(res);
  UNUSED(ret);

  fd = mkstemp(filename);
  assert(fd > 0);
  wfd = fdopen(fd, "a");
  assert(wfd != NULL);
  ret = fputs("test1\n", wfd);
  assert(ret > 0);
  ret = fflush(wfd);
  assert(ret == 0);

  res = f2b_logfile_open(&file, filename);
  assert(res == true);
  res = f2b_logfile_getline(&file, buf, sizeof(buf));
  assert(res == false);

  ret = fputs("test2\n", wfd);
  assert(ret > 0);
  ret = fflush(wfd);
  assert(ret == 0);

  res = f2b_logfile_getline(&file, buf, sizeof(buf));
  assert(res == true);
  ret = strncmp(buf, "test2\n", sizeof(buf));
  assert(ret == 0);

  res = f2b_logfile_getline(&file, buf, sizeof(buf));
  assert(res == false);

  fclose(wfd);
  close(fd);
  unlink(filename);

  wfd = fopen(filename, "a");
  assert(wfd != NULL);

  res = f2b_logfile_rotated(&file);
  assert(res == true);

  f2b_logfile_close(&file);
  fclose(wfd);
  unlink(filename);

  exit(0);
}
