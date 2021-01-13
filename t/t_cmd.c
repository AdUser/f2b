#include "../src/common.h"
#include "../src/buf.h"
#include "../src/commands.h"

int main() {
  f2b_cmd_t cmd;

  assert(f2b_cmd_parse(&cmd, "status") == true);
  assert(cmd.type = CMD_STATUS);
  assert(cmd.argc == 1);
  assert(strcmp(cmd.args[0], "status") == 0);
  assert(cmd.data.used == 6); /* "status" */
  f2b_buf_free(&cmd.data);

  assert(f2b_cmd_parse(&cmd, "stat") == false); /* no such command */
  assert(cmd.type == CMD_UNKNOWN);

  assert(f2b_cmd_parse(&cmd, "jail test") == false); /* incomplete command */
  assert(cmd.type == CMD_UNKNOWN);

  assert(f2b_cmd_parse(&cmd, "jail test status") == true);
  assert(cmd.type == CMD_JAIL_STATUS);
  assert(cmd.argc == 3);
  assert(strcmp(cmd.args[0], "jail") == 0);
  assert(strcmp(cmd.args[1], "test") == 0);
  assert(strcmp(cmd.args[2], "status") == 0);
  assert(cmd.data.used == 16); /* "jail\0test\0status" */
  f2b_buf_free(&cmd.data);

  assert(f2b_cmd_parse(&cmd, "jail test set bantime 7200") == true);
  assert(cmd.type == CMD_JAIL_SET);
  assert(cmd.argc == 5);
  assert(strcmp(cmd.args[0], "jail") == 0);
  assert(strcmp(cmd.args[1], "test") == 0);
  assert(strcmp(cmd.args[2], "set")  == 0);
  assert(strcmp(cmd.args[3], "bantime") == 0);
  assert(strcmp(cmd.args[4], "7200") == 0);
  f2b_buf_free(&cmd.data);

  return EXIT_SUCCESS;
}
