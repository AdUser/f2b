#include "common.h"
#include "log.h"
#include "ipaddr.h"
#include "regexps.h"

void usage() {
  fprintf(stderr, "Usage: filter-test <regexps-file.txt> <logfile.txt>\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  FILE *f = NULL;
  char matchbuf[IPADDR_MAX] = "";
  char logline[LOGLINE_MAX] = "";
  f2b_regex_t *list = NULL;
  size_t read = 0, matched = 0;

  if (argc < 3)
    usage();

  if ((list = f2b_regexlist_from_file(argv[1])) == NULL) {
    f2b_log_msg(log_error, "can't load regexps list from file '%s'", argv[1]);
    return EXIT_FAILURE;
  }

  if ((f = fopen(argv[2], "r")) == NULL) {
    f2b_log_msg(log_error, "can't open logfile '%s'", argv[2]);
    return EXIT_FAILURE;
  }

  while (fgets(logline, sizeof(logline), f) != NULL) {
    read++;
    if (f2b_regexlist_match(list, logline, matchbuf, sizeof(matchbuf))) {
      matched++;
      f2b_log_msg(log_info, "match found: %s", matchbuf);
      continue;
    }
    f2b_log_msg(log_info, "unmatched line: %s", logline);
  }
  f2b_log_msg(log_info, "lines read: %d, matched: %d", read, matched);

  return EXIT_SUCCESS;
}
