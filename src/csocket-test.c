#include "common.h"
#include "buf.h"
#include "log.h"
#include "commands.h"
#include "csocket.h"

static int run = 1;

void
cmd_handler(const f2b_cmd_t *cmd, f2b_buf_t *res) {
  fprintf(stdout, "[handler] received cmd with type %d and %d args:\n", cmd->type, cmd->argc);
  for (int i = 0; i < cmd->argc; i++) {
    fprintf(stdout, "[handler] arg %d : %s\n", i + 1, cmd->args[i]);
  }
  UNUSED(res);
  return;
}

int main(void) {
  f2b_csock_t *csock = NULL;

  if ((csock = f2b_csocket_create(DEFAULT_CSOCKET_PATH)) == NULL) {
    perror("f2b_csocket_create()");
  }

  while (run) {
    f2b_csocket_poll(csock, cmd_handler);
    /* TODO: sleep 0.1s */
  }
  f2b_csocket_destroy(csock);

  return 0;
}
