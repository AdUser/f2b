#include "../src/common.h"
#include "../src/commands.h"

int main() {
  char buf[1024];
  const char *line;

  UNUSED(line);

  buf[0] = '\0';
  f2b_cmd_append_arg(buf, sizeof(buf), "42");
  assert(strcmp(buf, "42\n") == 0);

  line = "status";
  assert(f2b_cmd_parse(buf, sizeof(buf), line) == CMD_STATUS);

  line = "statu"; /* no such command */
  assert(f2b_cmd_parse(buf, sizeof(buf), line) == CMD_NONE);

  line = "jail test"; /* incomplete command */
  assert(f2b_cmd_parse(buf, sizeof(buf), line) == CMD_NONE);

  buf[0] = '\0';
  line = "jail test status";
  assert(f2b_cmd_parse(buf, sizeof(buf), line) == CMD_JAIL_STATUS);
  assert(strcmp(buf, "test\n") == 0);

  buf[0] = '\0';
  line = "jail test set bantime 7200";
  assert(f2b_cmd_parse(buf, sizeof(buf), line) == CMD_JAIL_SET);
  assert(strcmp(buf, "test\nbantime\n7200\n") == 0);

  return EXIT_SUCCESS;
}
