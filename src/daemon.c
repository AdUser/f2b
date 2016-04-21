/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "logfile.h"
#include "ipaddr.h"
#include "config.h"
#include "jail.h"
#include "backend.h"
#include "cmsg.h"
#include "csocket.h"

#include <getopt.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>

/** global variables */
struct {
  bool daemon;
  uid_t uid;
  gid_t gid;
  int csock;
  char config_path[PATH_MAX];
  char logfile_path[PATH_MAX];
  char csocket_path[PATH_MAX];
  char pidfile_path[PATH_MAX];
} opts = {
  false,
  0, 0,
  -1,
  "/etc/f2b/f2b.conf",
  "/var/log/f2b.log",
  DEFAULT_CSOCKET_PATH,
  DEFAULT_PIDFILE_PATH,
};

enum { stop = 0, run, reconfig, test } state = run;
f2b_jail_t *jails = NULL;

void signal_handler(int signum) {
  switch (signum) {
    case SIGTERM:
    case SIGINT:
      f2b_log_msg(log_info, "got SIGTERM/SIGINT, exiting");
      state = stop;
      break;
    case SIGHUP:
      f2b_log_msg(log_note, "got SIGHUP, reloading config");
      state = reconfig;
      break;
    default:
      break;
  }
}

void usage(int exitcode) {
  fprintf(stderr, "Usage: f2b [-c <config>] [-d] [-h]\n");
  exit(exitcode);
}

#ifndef WITH_CSOCKET
/* add stubs to reduce #ifdef count */
int f2b_csocket_create (const char *path) {
  UNUSED(path);
  f2b_log_msg(log_warn, "control socket support was disabled at compile-time");
  return -1;
}
void f2b_csocket_destroy(int csock, const char *path) {
  UNUSED(csock); UNUSED(path); return;
}
int f2b_csocket_poll(int csock, void (*cb)(const f2b_cmsg_t *msg, char *res, size_t ressize)) {
  UNUSED(csock); UNUSED(cb); return 0;
}
void
f2b_cmsg_process(const f2b_cmsg_t *msg, char *res, size_t ressize) {
  UNUSED(msg); UNUSED(res); UNUSED(ressize); return;
}
#else /* WITH_CSOCKET */
void
f2b_cmsg_process(const f2b_cmsg_t *msg, char *res, size_t ressize) {
  const char *args[DATA_ARGS_MAX];
  const char *fmt;
  f2b_jail_t *jail = NULL;
  f2b_ipaddr_t *addr = NULL;
  char line[LINE_MAX];

  assert(msg != NULL);
  assert(res != NULL);
  assert(msg->type < CMD_MAX_NUMBER);

  memset(args, 0x0, sizeof(args));
  f2b_cmsg_extract_args(msg, args);

  if (msg->type >= CMD_JAIL_STATUS && msg->type <= CMD_JAIL_IP_RELEASE) {
    if (args[0] == NULL) {
      strlcpy(res, "can't find jail: no args\n", ressize);
      return;
    }
    if ((jail = f2b_jail_find(jails, args[0])) == NULL) {
      snprintf(res, ressize, "can't find jail '%s'\n", args[0]);
      return;
    }
  }

  if (jail && (msg->type >= CMD_JAIL_IP_SHOW && msg->type <= CMD_JAIL_IP_RELEASE)) {
    if (args[1] == NULL) {
      strlcpy(res, "can't find ip: no args", ressize);
      return;
    }
    if ((addr = f2b_addrlist_lookup(jail->ipaddrs, args[1])) == NULL) {
      snprintf(res, ressize, "can't find ip '%s' in jail '%s'\n", args[1], args[0]);
      return;
    }
  }

  if (msg->type == CMD_PING) {
    strlcpy(res, "ok", ressize);
  } else if (msg->type == CMD_RELOAD) {
    state = reconfig;
    strlcpy(res, "ok", ressize);
  } else if (msg->type == CMD_SHUTDOWN) {
    state = stop;
    strlcpy(res, "ok", ressize);
  } else if (msg->type == CMD_STATUS) {
    snprintf(line, sizeof(line), "pid: %u\npidfile: %s\ncsocket: %s\njails:\n",
      getpid(), opts.pidfile_path, opts.csocket_path);
    strlcpy(res, line, ressize);
    for (jail = jails; jail != NULL; jail = jail->next) {
      snprintf(line, sizeof(line), "- %s\n", jail->name);
      strlcat(res, line, ressize);
    }
  } else if (msg->type == CMD_JAIL_STATUS) {
    fmt = "name: %s\n"
          "enabled: %s\n"
          "maxretry: %d\n"
          "times:\n"
          "  bantime: %d\n"
          "  findtime: %d\n"
          "  expiretime: %d\n"
          "incr:\n"
          "  bantime: %.1f\n"
          "  findtime: %.1f\n"
          "stats:\n"
          "  banned: %d\n"
          "  matched: %d\n";
    snprintf(res, ressize, fmt, jail->name, jail->enabled ? "yes" : "no", jail->maxretry,
      jail->bantime, jail->findtime, jail->expiretime,
      jail->incr_bantime, jail->incr_findtime,
      jail->bancount, jail->matchcount);
  } else if (msg->type == CMD_JAIL_IP_SHOW) {
    fmt = "ipaddr: %s\n"
          "banned: %s\n"
          "bancount: %d\n"
          "lastseen: %d\n"
          "banned_at: %d\n"
          "release_at: %d\n";
    snprintf(res, ressize, fmt, addr->text, addr->banned ? "yes" : "no",
      addr->bancount, addr->lastseen, addr->banned_at, addr->release_at);
  } else if (msg->type == CMD_JAIL_IP_BAN) {
    f2b_jail_ban(jail, addr);
    strlcpy(res, "ok", ressize);
  } else if (msg->type == CMD_JAIL_IP_RELEASE) {
    f2b_jail_unban(jail, addr);
    strlcpy(res, "ok", ressize);
  } else {
    strlcpy(res, "error: unsupported command type", ressize);
  }

  return;
}
#endif /* WITH_CSOCKET */

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

  if ((pa = f2b_config_param_find(section->param, "pidfile")) != NULL)
    strlcpy(opts.pidfile_path, pa->value, sizeof(opts.pidfile_path));

  if ((pa = f2b_config_param_find(section->param, "csocket")) != NULL)
    strlcpy(opts.csocket_path, pa->value, sizeof(opts.csocket_path));

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
        strlcpy(opts.logfile_path, pb->value, sizeof(opts.logfile_path));
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
  f2b_jail_t *jail  = NULL;
  char opt = '\0';

  while ((opt = getopt(argc, argv, "c:dht")) != -1) {
    switch (opt) {
      case 'c':
        strlcpy(opts.config_path, optarg, sizeof(opts.config_path));
        break;
      case 'd':
        opts.daemon = true;
        break;
      case 'h':
        usage(EXIT_SUCCESS);
        break;
      case 't':
        state = test;
        break;
      default:
        usage(EXIT_FAILURE);
        break;
    }
  }

  SA_REGISTER(SIGTERM, &signal_handler);
  SA_REGISTER(SIGINT,  &signal_handler);
  SA_REGISTER(SIGHUP,  &signal_handler);

  if (opts.config_path[0] == '\0')
    usage(EXIT_FAILURE);
  memset(&config, 0x0, sizeof(config));
  if (f2b_config_load(&config, opts.config_path, true) != true) {
    f2b_log_msg(log_error, "can't load config from '%s'", opts.config_path);
    return EXIT_FAILURE;
  }
  if (state == test) {
    fprintf(stderr, "config test ok\n");
    exit(EXIT_SUCCESS);
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

  if (opts.pidfile_path[0] != '\0') {
    FILE *pidfile = NULL;
    if ((pidfile = fopen(opts.pidfile_path, "w")) != NULL) {
      if (flock(fileno(pidfile), LOCK_EX | LOCK_NB) != 0) {
        const char *err = (errno == EWOULDBLOCK)
          ? "another instance already running"
          : strerror(errno);
        f2b_log_msg(log_error, "can't lock pidfile: %s", err);
        exit(EXIT_FAILURE);
      }
      fprintf(pidfile, "%d\n", getpid());
    } else {
      f2b_log_msg(log_warn, "can't open pidfile: %s", strerror(errno));
    }
  }

  if (opts.csocket_path[0] != '\0')
    opts.csock = f2b_csocket_create(opts.csocket_path);

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
    f2b_log_msg(log_warn, "no jails configured, exiting");
    return EXIT_FAILURE;
  }

  while (state) {
    for (jail = jails; jail != NULL; jail = jail->next) {
      f2b_jail_process(jail);
    }
    f2b_csocket_poll(opts.csock, f2b_cmsg_process);
    sleep(1);
    if (state == reconfig) {
      /* TODO */
      state = run;
    }
  }

  f2b_csocket_destroy(opts.csock, opts.csocket_path);

  for (jail = jails; jail != NULL; jail = jail->next)
    f2b_jail_stop(jail);

  return EXIT_SUCCESS;
}