#include "common.h"
#include "logfile.h"
#include "ipaddr.h"
#include "config.h"
#include "jail.h"
#include "backend.h"

#include <getopt.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>

struct {
  bool daemon;
  uid_t uid;
  gid_t gid;
  char config_path[PATH_MAX];
  char logfile_path[PATH_MAX];
  char pidfile_path[PATH_MAX];
} opts = {
  false,
  0, 0,
  "/etc/f2b/f2b.conf",
  "/var/log/f2b.log",
  "/var/run/f2b.pid",
};

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

#define SA_REGISTER(SIGNUM, HANDLER) \
  memset(&act, 0x0, sizeof(act)); \
  act.sa_handler = HANDLER; \
  if (sigaction(SIGNUM, &act, NULL) != 0) { \
    f2b_log_msg(log_error, "can't register handler for " #SIGNUM); \
    return EXIT_FAILURE; \
  }

void usage(int exitcode) {
  fprintf(stderr, "Usage: f2b [-c <config>] [-d] [-h]\n");
  exit(exitcode);
}

void
update_opts_from_config(f2b_config_section_t *section) {
  f2b_config_param_t *pa, *pb;

  if (!section)
    return;

  /* set uid & gid. note: set only once if root */
  if (opts.uid == 0 && (pa = f2b_config_param_find(section->param, "user")) != NULL) {
    struct passwd *pw;
    if ((pw = getpwnam(pa->value)) != NULL)
      opts.uid = pw->pw_uid, opts.gid = pw->pw_gid;
  }
  if (opts.gid == 0 && (pa = f2b_config_param_find(section->param, "group")) != NULL) {
    struct group *grp;
    if ((grp = getgrnam(pa->value)) != NULL)
      opts.gid = grp->gr_gid;
  }

  if (opts.daemon == false && (pa = f2b_config_param_find(section->param, "daemon")) != NULL) {
    if (strcmp(pa->value, "yes") == 0)
      opts.daemon = true;
  }

  /* setup logging */
  if ((pa = f2b_config_param_find(section->param, "loglevel")) != NULL)
    f2b_log_set_level(pa->value);

  pa = f2b_config_param_find(section->param, "logdest");
  pb = f2b_config_param_find(section->param, "logfile");
  if (pa) {
    if (!opts.daemon && strcmp(pa->value, "stderr") == 0) {
      f2b_log_to_stderr();
    } else if (strcmp(pa->value, "file") == 0) {
      if (pb && *pb->value != '\0') {
        size_t len = sizeof(opts.logfile_path);
        strncpy(opts.logfile_path, pb->value, len - 1);
        opts.logfile_path[len - 1] = '\0';
        f2b_log_to_file(opts.logfile_path);
      } else {
        f2b_log_msg(log_warn, "you must set 'logfile' option with 'logdest = file'");
        f2b_log_to_syslog();
      }
    } else {
      f2b_log_to_syslog();
    }
  }

  /* TODO: */
}

int main(int argc, char *argv[]) {
  struct sigaction act;
  f2b_config_t config;
  f2b_config_section_t *section = NULL;
  f2b_jail_t *jails = NULL;
  f2b_jail_t *jail  = NULL;
  char opt = '\0';

  while ((opt = getopt(argc, argv, "c:dh")) != -1) {
    switch (opt) {
      case 'c':
        strncpy(opts.config_path, optarg, sizeof(opts.config_path));
        break;
      case 'd':
        opts.daemon = true;
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

  if (opts.config_path[0] == '\0')
    usage(EXIT_FAILURE);
  memset(&config, 0x0, sizeof(config));
  if (f2b_config_load(&config, opts.config_path, true) != true) {
    f2b_log_msg(log_error, "can't load config from '%s'", opts.config_path);
    return EXIT_FAILURE;
  }
  update_opts_from_config(config.main);

  if (opts.daemon) {
    pid_t pid = fork();
    if (pid > 0)
      exit(EXIT_SUCCESS);
      /* parent */
    if (pid < 0) {
      /* parent */
      perror("child: fork() failed");
      exit(EXIT_FAILURE);
    }
    /* child */
    setsid();
    if (getuid() == 0 &&
        (setuid(opts.uid) != 0 ||
         setgid(opts.gid) != 0)) {
      perror("child: setuid()/setgid() failed");
      exit(EXIT_FAILURE);
    }
    if (chdir("/") != 0 ||
        freopen("/dev/null", "r", stdin)  == NULL ||
        freopen("/dev/null", "w", stdout) == NULL ||
        freopen("/dev/null", "w", stderr) == NULL) {
      perror("child: freopen() failed");
      exit(EXIT_FAILURE);
    }
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
    if (rcfg) {
      /* TODO */
      rcfg = false;
    }
  }

  for (jail = jails; jail != NULL; jail = jail->next)
    f2b_jail_stop(jail);

  return EXIT_SUCCESS;
}
