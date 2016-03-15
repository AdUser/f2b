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
  fprintf(stderr, "Usage: filter-test <library.so> <regexps-file.txt>\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  f2b_config_param_t  param = { .name = "load", .value = "", .next = 0x0 };
  f2b_config_section_t config = { .type = t_filter, .param = 0x0, .next = 0x0 };
  f2b_filter_t *filter = NULL;
  char match[IPADDR_MAX] = "";
  char line[LOGLINE_MAX] = "";
  size_t read = 0, matched = 0;

  if (argc < 3)
    usage();

  config.param = &param;
  snprintf(param.value, sizeof(param.value), "%s", argv[1]);

  if ((filter = f2b_filter_create(&config, argv[2])) == false)
    usage();

  while (fgets(line, sizeof(line), stdin) != NULL) {
    read++;
    if (f2b_filter_match(filter, line, match, sizeof(match))) {
      matched++;
      fprintf(stderr, "+ %s\n", match);
      continue;
    }
    fprintf(stderr, "- (no-match): %s", line);
  }
  fprintf(stderr, "%% lines read: %d, matched: %d\n", read, matched);

  return EXIT_SUCCESS;
}
