/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "log.h"
#include "ipaddr.h"
#include "config.h"
#include "filter.h"

void usage() {
  fprintf(stderr, "Usage: filter-test <filter.conf> <regexps.txt> [<file.log>]\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  f2b_config_t          config;
  f2b_config_section_t *section = NULL;
  f2b_filter_t         *filter  = NULL;
  char match[IPADDR_MAX] = "";
  char line[LOGLINE_MAX] = "";
  char stats[4096];
  size_t read = 0, matched = 0;
  FILE *file = NULL;

  if (argc < 3)
    usage();

  memset(&config, 0x0, sizeof(config));
  if (f2b_config_load(&config, argv[1], false) != true) {
    f2b_log_msg(log_fatal, "can't load config");
    return EXIT_FAILURE;
  }

  if (config.filters == NULL) {
    f2b_log_msg(log_fatal, "no filters found in config");
    return EXIT_FAILURE;
  } else {
    section = config.filters;
  }

  if ((filter = f2b_filter_create(section, argv[2])) == false) {
    f2b_log_msg(log_fatal, "can't create filter '%s' with file '%s'", section->name, argv[2]);
    return EXIT_FAILURE;
  }

  if (argc > 3) {
    if ((file = fopen(argv[3], "r")) == NULL) {
      f2b_log_msg(log_fatal, "can't open regexp file '%s': %s", argv[2], strerror(errno));
      return EXIT_FAILURE;
    }
  } else {
    file = stdin;
  }

  while (fgets(line, sizeof(line), file) != NULL) {
    read++;
    if (f2b_filter_match(filter, line, match, sizeof(match))) {
      matched++;
      fprintf(stdout, "+ %s\n", match);
      continue;
    } else {
      fprintf(stdout, "- (no-match): %s", line);
    }
  }
  fclose(file);

  fputs("---\n", stdout);
  fprintf(stdout, "stats: %zu lines read, %zu matched\n", read, matched);
  f2b_filter_cmd_stats(stats, sizeof(stats), filter);
  fputs(stats, stdout);

  return EXIT_SUCCESS;
}
