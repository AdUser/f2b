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
  fprintf(stderr, "Usage: source-test <source.conf> <init string>\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  f2b_config_t          config;
  f2b_config_section_t *section = NULL;
  f2b_source_t         *source  = NULL;
  char buf[1024] = "";
  bool reset;

  if (argc < 3)
    usage();

  memset(&config, 0x0, sizeof(config));
  if (f2b_config_load(&config, argv[1], false) != true) {
    f2b_log_msg(log_error, "can't load config");
    return EXIT_FAILURE;
  }

  if (config.sources == NULL) {
    f2b_log_msg(log_error, "no sources found in config");
    return EXIT_FAILURE;
  } else {
    section = config.sources;
  }

  if ((source = f2b_source_create(section, argv[2], f2b_log_error_cb)) == NULL) {
    f2b_log_msg(log_error, "can't create source '%s' with init '%s'", section->name, argv[2]);
    return EXIT_FAILURE;
  }

  if (f2b_source_start(source) == false) {
    f2b_log_msg(log_error, "source start error: %s", f2b_source_error(source));
    exit(EXIT_FAILURE);
  }

  while (1) {
    reset = true;
    while (f2b_source_next(source, buf, sizeof(buf), reset)) {
      reset = false;
      puts(buf);
    }
    sleep(1);
  }

  return EXIT_SUCCESS;
}
