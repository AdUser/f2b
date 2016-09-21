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
  fprintf(stderr, "Usage: filter-test <library.so> <regexps-file.txt> [<file.log>]\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  f2b_config_param_t  param = { .name = "load", .value = "", .next = 0x0 };
  f2b_config_section_t config = { .name = "test", .type = t_filter, .param = 0x0, .next = 0x0 };
  f2b_filter_t *filter = NULL;
  char match[IPADDR_MAX] = "";
  char line[LOGLINE_MAX] = "";
  char stats[4096];
  size_t read = 0, matched = 0;
  const char *error;
  FILE *file = NULL;

  if (argc < 3)
    usage();

  config.param = &param;
  snprintf(param.value, sizeof(param.value), "%s", argv[1]);

  if ((filter = f2b_filter_create(&config, argv[2])) == false)
    usage();

  if (argc > 3) {
    if ((file = fopen(argv[3], "r")) == NULL)
      usage();
  } else {
    file = stdin;
  }

  while (fgets(line, sizeof(line), file) != NULL) {
    read++;
    if (f2b_filter_match(filter, line, match, sizeof(match))) {
      matched++;
      fprintf(stderr, "+ %s\n", match);
      continue;
    }
    error = f2b_filter_error(filter);
    if (*error == '\0') {
      fprintf(stderr, "- (no-match): %s", line);
    } else {
      fprintf(stderr, "! (error) : %s\n", error);
    }
  }
  fclose(file);
  fprintf(stderr, "stats: %% lines read: %zu, matched: %zu\n", read, matched);
  f2b_filter_cmd_stats(stats, sizeof(stats), filter);
  fputs(stats, stderr);

  return EXIT_SUCCESS;
}
