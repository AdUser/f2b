#include "common.h"
#include "logfile.h"
#include "ipaddr.h"
#include "regexps.h"
#include "config.h"
#include "jail.h"
#include "backend.h"

#include <getopt.h>

void usage(int exitcode) {
  fprintf(stderr, "Usage: f2b -c <config>\n");
  exit(exitcode);
}

int main(int argc, char *argv[]) {
  f2b_config_t *config  = NULL;
  f2b_config_section_t *section = NULL;
  f2b_jail_t *jails = NULL;
  f2b_jail_t *jail  = NULL;
  char *config_file = NULL;
  char opt = '\0';
  bool run = true;

  while ((opt = getopt(argc, argv, "c:h")) != -1) {
    switch (opt) {
      case 'c':
        config_file = optarg;
        break;
      case 'h':
        usage(EXIT_SUCCESS);
        break;
      default:
        usage(EXIT_FAILURE);
        break;
    }
  }

  if (!config_file)
    usage(EXIT_FAILURE);
  if ((config = f2b_config_load(config_file)) == NULL) {
    f2b_log_msg(log_error, "can't load config from '%s'", config_file);
    return EXIT_FAILURE;
  }

  if (config->defaults)
    f2b_jail_set_defaults(config->defaults);

  for (section = config->jails; section != NULL; section = section->next) {
    if ((jail = f2b_jail_create(section)) == NULL) {
      f2b_log_msg(log_error, "can't create jail '%s'", section->name);
      continue;
    }
    if (!jail->enabled) {
      f2b_log_msg(log_debug, "ignoring disabled jail '%s'", jail->name);
      free(jail);
      continue;
    }
    if (!f2b_jail_init(jail, config)) {
      f2b_log_msg(log_error, "can't init jail '%s'", section->name);
      free(jail);
      continue;
    }
    jail->next = jails;
    jails = jail;
  }
  f2b_config_free(config);

  if (!jails) {
    f2b_log_msg(log_error, "no jails configured");
    return EXIT_FAILURE;
  }

  while (run) {
    for (jail = jails; jail != NULL; jail = jail->next) {
      f2b_jail_process(jail);
    }
    sleep(1);
  }

  return EXIT_SUCCESS;
}
