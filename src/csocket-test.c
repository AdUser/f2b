#include "common.h"
#include "config.h"
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
  if (cmd->type == CMD_HELP) {
    f2b_buf_append(res, f2b_cmd_help(), 0);
  }
  return;
}

int main(int argc, const char **argv) {
  f2b_config_t config;
  f2b_csock_t *csock = NULL;

  if (argc < 2) {
    puts("Usage: csocket-test <csocket.conf>");
    exit(EXIT_FAILURE);
  }

  f2b_log_set_level("debug");
  f2b_log_to_stderr();

  memset(&config, 0x0, sizeof(config));
  if (!f2b_config_load(&config, argv[1], false)) {
    f2b_log_msg(log_fatal, "can't load config");
    return EXIT_FAILURE;
  }

  if ((csock = f2b_csocket_create(config.csocket)) == NULL) {
    perror("f2b_csocket_create()");
    exit(EXIT_FAILURE);
  }

  while (run) {
    f2b_csocket_poll(csock, cmd_handler);
    sleep(1);
  }
  f2b_csocket_destroy(csock);

  return 0;
}
