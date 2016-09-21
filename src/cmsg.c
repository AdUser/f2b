#include "common.h"
#include "commands.h"
#include "cmsg.h"

#include <sys/uio.h>

void
f2b_cmsg_convert_args(f2b_cmsg_t *msg) {
  assert(msg != NULL);

  for (size_t i = 0; i < msg->size && i < sizeof(msg->data); i++) {
    if (msg->data[i] == '\n')
      msg->data[i] = '\0';
  }
}

int
f2b_cmsg_extract_args(const f2b_cmsg_t *msg, const char **argv) {
  char prev = '\0';
  size_t argc = 0;

  assert(msg  != NULL);
  assert(argv != NULL);

  if (msg->size == 0)
    return 0; /* no args */

  if (msg->data[msg->size - 1] != '\0')
    return -1; /* message data not null-terminated */

  for (size_t i = 0; i < msg->size; i++) {
    if (prev == '\0' && msg->data[i] != '\0')
      argv[argc] = &msg->data[i], argc++;
    if (argc >= DATA_ARGS_MAX) {
      argc = -1;
      break;
    }
    prev = msg->data[i];
  }

  return argc;
}
