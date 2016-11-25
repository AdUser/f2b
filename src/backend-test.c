/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "config.h"
#include "log.h"
#include "backend.h"

void usage() {
  fprintf(stderr, "Usage: backend-test <backend.conf> <id>\n");
  exit(EXIT_FAILURE);
}

/* returns -1 on error, 0 for 'ban', 1 for 'check' and 3 for 'unban'
 * also fills buf with second arg after command name
 */
int
parse_input(char *line, char *buf, size_t bufsize) {
  const char *ops[] = { "ban", "check", "unban", NULL };
  int code = 0;

  assert(line != NULL);
  assert(buf  != NULL);

  while (isspace(*line))
    line++;
  if (*line == '\0')
    return -1; /* empty line */

  for (size_t i = 0; ops[i] != NULL; i++) {
    size_t len = strlen(ops[i]);
    if (strncmp(line, ops[i], len) != 0)
      continue;
    code = i;
    line += len;
    break;
  }

  while (isspace(*line))
    line++;
  if (*line == '\0')
    return -1; /* missing second arg */

  for (char *p = line; *p != '\0'; p++) {
    if ((*p >= 'a' && *p <= 'f') || *p == '.' || *p == ':' ||
        (*p >= 'A' && *p <= 'F') || isdigit(*p))
      continue; /* valid ipaddr char */
    /* everything else */
    *p = '\0';
    break;
  }

  if (*line == '\0')
    return -1; /* garbled second arg */

  strlcpy(buf, line, bufsize);
  return code;
}

int main(int argc, char *argv[]) {
  const char *ip = "127.0.0.17";
  f2b_config_t          config;
  f2b_config_section_t *section = NULL;
  f2b_backend_t        *backend = NULL;

  if (argc < 3)
    usage();

  memset(&config, 0x0, sizeof(config));
  if (f2b_config_load(&config, argv[1], false) != true) {
    f2b_log_msg(log_fatal, "can't load config");
    return EXIT_FAILURE;
  }

  if (config.backends == NULL) {
    f2b_log_msg(log_fatal, "no backends found in config");
    return EXIT_FAILURE;
  } else {
    section = config.backends;
  }

  if ((backend = f2b_backend_create(section, argv[2])) == NULL) {
    f2b_log_msg(log_fatal, "can't create backend '%s' with id '%s'", section->name, argv[2]);
    return EXIT_FAILURE;
  }

  if (!f2b_backend_start(backend)) {
    f2b_log_msg(log_error, "action 'ban' failed: %s", f2b_backend_error(backend));
    goto cleanup;
  }

  if (!f2b_backend_ban(backend, ip)) {
    f2b_log_msg(log_error, "action 'ban' failed: %s", f2b_backend_error(backend));
    goto cleanup;
  }

  if (f2b_backend_check(backend, ip)) {
    f2b_log_msg(log_info, "action 'check' failed returned true");
  } else {
    f2b_log_msg(log_info, "action 'check' failed returned false");
  }

  if (!f2b_backend_unban(backend, ip)) {
    f2b_log_msg(log_error, "action 'unban' failed: %s", f2b_backend_error(backend));
    goto cleanup;
  }

  f2b_log_msg(log_info, "all tests passed");

  cleanup:
  f2b_backend_stop(backend);
  f2b_backend_destroy(backend);
  f2b_config_free(&config);

  return EXIT_SUCCESS;
}
