#include "../src/common.h"
#include "../src/cmsg.h"

int main() {
  const char *argv[DATA_ARGS_MAX];
  f2b_cmsg_t msg;

  memset(&msg, 0x0, sizeof(msg));
  memset(argv, 0x0, sizeof(argv));

  snprintf(msg.data, sizeof(msg.data), "test1\ntest2\n");
  msg.size = strlen(msg.data);

  f2b_cmsg_convert_args(&msg);
  assert(memcmp(msg.data, "test1\0test2\0", 12) == 0);

  f2b_cmsg_extract_args(&msg, argv);
  assert(argv[0] == msg.data + 0);
  assert(argv[1] == msg.data + 6);
  assert(memcmp(argv[0], "test1\0", 6) == 0);
  assert(memcmp(argv[1], "test2\0", 6) == 0);

  /* data not null-terminated */
  msg.size = 10;
  memcpy(msg.data, "test1\0test2\n", 10);
  assert(f2b_cmsg_extract_args(&msg, argv) == -1);

  msg.size = 0;
  assert(f2b_cmsg_extract_args(&msg, argv) == 0);

  return EXIT_SUCCESS;
}
