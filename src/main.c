#include "common.h"
#include "logfile.h"
#include "ipaddr.h"
#include "config.h"
#include "jail.h"
#include "backend.h"

#include <getopt.h>
#include <signal.h>

bool run  = true;
bool rcfg = false;

void sa_term(int signum) {
  UNUSED(signum);
  f2b_log_msg(log_info, "got SIGTERM, exiting");
  run = false;
}
void sa_hup(int signum) {
  UNUSED(signum);
  f2b_log_msg(log_info, "got SIGHUP, reloading config");
  rcfg = true;
}

#define SA_REGISTER(signum, handler) \
  memset(&act, 0x0, sizeof(act)); \
  act.sa_handler = handler; \
  if (sigaction(SIGTERM, &act, NULL) != 0) { \
    f2b_log_msg(log_error, "can't register handler for " #signum); \
    return EXIT_FAILURE; \
  }

void usage(int exitcode) {
  fprintf(stderr, "Usage: f2b -c <config>\n");
  exit(exitcode);
}

int main(int argc, char *argv[]) {
  struct sigaction act;
  f2b_config_t config;
  f2b_config_section_t *section = NULL;
  f2b_jail_t *jails = NULL;
  f2b_jail_t *jail  = NULL;
  char *config_file = NULL;
  char opt = '\0';

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

  SA_REGISTER(SIGTERM, &sa_term);
  SA_REGISTER(SIGHUP,  &sa_hup);

  if (!config_file)
    usage(EXIT_FAILURE);
  memset(&config, 0x0, sizeof(config));
  if (f2b_config_load(&config, config_file, true) != true) {
    f2b_log_msg(log_error, "can't load config from '%s'", config_file);
    return EXIT_FAILURE;
  }

  if (config.defaults)
    f2b_jail_set_defaults(config.defaults);

  for (section = config.jails; section != NULL; section = section->next) {
    if ((jail = f2b_jail_create(section)) == NULL) {
      f2b_log_msg(log_error, "can't create jail '%s'", section->name);
      continue;
    }
    if (!jail->enabled) {
      f2b_log_msg(log_debug, "ignoring disabled jail '%s'", jail->name);
      free(jail);
      continue;
    }
    if (!f2b_jail_init(jail, &config)) {
      f2b_log_msg(log_error, "can't init jail '%s'", section->name);
      free(jail);
      continue;
    }
    jail->next = jails;
    jails = jail;
  }
  f2b_config_free(&config);

  if (!jails) {
    f2b_log_msg(log_error, "no jails configured, exiting");
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
