#include "../src/common.h"
#include "../src/buf.h"

int main() {
  f2b_buf_t buf;
  char *line;
  bool ret;
  int len;

  memset(&buf, 0x0, sizeof(buf));

  ret = f2b_buf_alloc(&buf, 512);
  assert(ret = true);
  assert(buf.used == 0);
  assert(buf.size == 512);

  ret = f2b_buf_append(&buf, "test ", 0);
  assert(ret == true);
  assert(buf.used == 5);

  line = f2b_buf_extract(&buf, "\n");
  assert(line == NULL);

  ret = f2b_buf_append(&buf, "test2\n", 0);
  assert(ret == true);
  assert(buf.used == 11);

  ret = f2b_buf_append(&buf, "test3\n", 0);
  assert(ret == true);
  assert(buf.used == 17);

  line = f2b_buf_extract(&buf, "\n");
  assert(line != NULL);
  assert(strcmp(line, "test test2") == 0);
  assert(buf.used == 6);
  free(line);

  line = f2b_buf_extract(&buf, "\n");
  assert(line != NULL);
  assert(buf.used == 0);
  free(line);

  f2b_buf_append(&buf, "test4\n\n", 6);
  assert(buf.used == 6);
  assert(strcmp(buf.data, "test4\n") == 0);

  len = f2b_buf_splice(&buf, 0);
  assert(len == 0);
  assert(buf.used == 6);
  assert(strcmp(buf.data, "test4\n") == 0);

  len = f2b_buf_splice(&buf, 2);
  assert(len == 2);
  assert(buf.used == 4);
  assert(strcmp(buf.data, "st4\n") == 0);

  len = f2b_buf_splice(&buf, 6);
  assert(len == 4);
  assert(buf.used == 0);
  assert(buf.data[0] == '\0');

  f2b_buf_free(&buf);
  assert(buf.used == 0);
  assert(buf.size == 0);
  assert(buf.data == NULL);

  return 0;
}
