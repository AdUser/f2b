/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "log.h"
#include "config.h"
#include "source.h"

void usage() {
  fprintf(stderr, "Usage: source-test <library.so> <init string>\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  f2b_config_param_t  param = { .name = "load", .value = "", .next = 0x0 };
  f2b_config_section_t config = { .name = "test", .type = t_source, .param = 0x0, .next = 0x0 };
  f2b_source_t *source = NULL;
  char buf[1024] = "";
  bool reset;

  if (argc < 2)
    usage();

  config.param = &param;
  snprintf(param.value, sizeof(param.value), "%s", argv[1]);

  if ((source = f2b_source_create(&config, argv[2], f2b_log_error_cb)) == false)
    usage();

  if (f2b_source_start(source) == false) {
    f2b_log_msg(log_error, "source start error: %s", f2b_source_error(source));
    exit(EXIT_FAILURE);
  }

  while (1) {
    reset = true;
    while (f2b_source_next(source, buf, sizeof(buf), reset)) {
      reset = false;
      fputs(buf, stderr);
    }
    sleep(1);
  }

  return EXIT_SUCCESS;
}
