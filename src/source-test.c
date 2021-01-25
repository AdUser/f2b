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

#include <signal.h>

bool run = 1;

void sigint_handler (int signal) {
  UNUSED(signal);
  run = 0;
}

void usage() {
  fprintf(stderr, "Usage: source-test <source.conf> <init string>\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  struct sigaction act;
  f2b_config_t          config;
  f2b_config_section_t *section = NULL;
  f2b_source_t         *source  = NULL;
  char buf[1024] = "";
  uint32_t stag;
  bool reset;

  if (argc < 3)
    usage();

  memset(&config, 0x0, sizeof(config));
  if (f2b_config_load(&config, argv[1], false) != true) {
    f2b_log_msg(log_fatal, "can't load config");
    return EXIT_FAILURE;
  }

  if (config.sources == NULL) {
    f2b_log_msg(log_fatal, "no sources found in config");
    return EXIT_FAILURE;
  } else {
    section = config.sources;
  }

  if ((source = f2b_source_create(section->name, argv[2])) == NULL) {
    f2b_log_msg(log_fatal, "can't create source '%s'", section->name);
    return EXIT_FAILURE;
  }

  if (!f2b_source_init(source, section)) {
    f2b_log_msg(log_fatal, "can't init source '%s'", source->name);
    return EXIT_FAILURE;
  }

  if (f2b_source_start(source) == false) {
    f2b_log_msg(log_fatal, "source start error");
    exit(EXIT_FAILURE);
  }

  memset(&act, 0x0, sizeof(act));
  act.sa_handler = sigint_handler;
  if (sigaction(SIGINT, &act, NULL) != 0) {
    f2b_log_msg(log_fatal, "can't register handler for SIGINT");
    return EXIT_FAILURE;
  }

  while (run) {
    reset = true;
    while ((stag = f2b_source_next(source, buf, sizeof(buf), reset)) > 0) {
      reset = false;
      printf("stag: %08X, data: %s\n", stag, buf);
    }
    sleep(1);
  }

  return EXIT_SUCCESS;
}
