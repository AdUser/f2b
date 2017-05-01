/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "ipaddr.h"
#include "config.h"
#include "jail.h"
#include "backend.h"
#include "appconfig.h"
#include "commands.h"
#include "cmsg.h"
#include "csocket.h"

#include <getopt.h>
#include <signal.h>

enum { stop = 0, run, reconfig, logrotate, test } state = run;

void signal_handler(int signum) {
  switch (signum) {
    case SIGUSR1:
      f2b_log_msg(log_info, "got SIGUSR1, reopening log file");
      state = logrotate;
      break;
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
  fprintf(stderr, "Usage: f2b [-c <config>] [-d] [-h] [-t]\n");
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
  f2b_jail_t *jail = NULL;
  char line[LINE_MAX];

  assert(msg != NULL);
  assert(res != NULL);
  assert(msg->type < CMD_MAX_NUMBER);

  if (msg->type == CMD_NONE)
    return;

  memset(args, 0x0, sizeof(args));
  int argc = f2b_cmsg_extract_args(msg, args);

  if (f2b_cmd_check_argc(msg->type, argc) == false) {
    strlcpy(res, "cmd args number mismatch", ressize);
    return;
  }

  if (msg->type >= CMD_JAIL_STATUS && msg->type <= CMD_MAX_NUMBER) {
    if ((jail = f2b_jail_find(jails, args[0])) == NULL) {
      snprintf(res, ressize, "can't find jail '%s'", args[0]);
      return;
    }
  }

  strlcpy(res, "ok", ressize); /* default reply */
  if (msg->type == CMD_PING) {
    /* nothing to do */
  } else if (msg->type == CMD_RELOAD) {
    state = reconfig;
  } else if (msg->type == CMD_LOG_ROTATE) {
    state = logrotate;
  } else if (msg->type == CMD_LOG_LEVEL) {
    f2b_log_set_level(args[0]);
  } else if (msg->type == CMD_SHUTDOWN) {
    state = stop;
  } else if (msg->type == CMD_STATUS) {
    snprintf(line, sizeof(line), "pid: %u\npidfile: %s\ncsocket: %s\nstatedir: %s\njails:\n",
      getpid(), appconfig.pidfile_path, appconfig.csocket_path, appconfig.statedir_path);
    strlcpy(res, line, ressize);
    for (jail = jails; jail != NULL; jail = jail->next) {
      snprintf(line, sizeof(line), "- %s\n", jail->name);
      strlcat(res, line, ressize);
    }
  } else if (msg->type == CMD_JAIL_STATUS) {
    f2b_jail_cmd_status(res, ressize, jail);
  } else if (msg->type == CMD_JAIL_SET) {
    f2b_jail_cmd_set(res, ressize, jail, args[1], args[2]);
  } else if (msg->type == CMD_JAIL_IP_STATUS) {
    f2b_jail_cmd_ip_xxx(res, ressize, jail,  0, args[1]);
  } else if (msg->type == CMD_JAIL_IP_BAN) {
    f2b_jail_cmd_ip_xxx(res, ressize, jail,  1, args[1]);
  } else if (msg->type == CMD_JAIL_IP_RELEASE) {
    f2b_jail_cmd_ip_xxx(res, ressize, jail, -1, args[1]);
  } else if (msg->type == CMD_JAIL_FILTER_STATS) {
    f2b_filter_cmd_stats(res, ressize, jail->filter);
  } else if (msg->type == CMD_JAIL_FILTER_RELOAD) {
    f2b_filter_cmd_reload(res, ressize, jail->filter);
  } else {
    strlcpy(res, "error: unsupported command type", ressize);
  }

  return;
}
#endif /* WITH_CSOCKET */

void
jails_start(f2b_config_t *config) {
  f2b_jail_t *jail = NULL;
  f2b_config_section_t *jail_config = NULL;

  assert(config != NULL);

  for (jail_config = config->jails; jail_config != NULL; jail_config = jail_config->next) {
    if ((jail = f2b_jail_create(jail_config)) == NULL) {
      f2b_log_msg(log_error, "can't create jail '%s'", jail_config->name);
      continue;
    }
    if (!(jail->flags & JAIL_ENABLED)) {
      f2b_log_msg(log_debug, "ignoring disabled jail '%s'", jail->name);
      free(jail);
      continue;
    }
    if (!f2b_jail_init(jail, config)) {
      f2b_log_msg(log_error, "can't init jail '%s'", jail_config->name);
      free(jail);
      continue;
    }

    f2b_jail_start(jail);

    jail->next = jails;
    jails = jail;
  }
}

void
jails_stop(f2b_jail_t *jails) {
  f2b_jail_t *jail = jails;
  f2b_jail_t *next = NULL;
  for (; jail != NULL; ) {
    next = jail->next;
    f2b_jail_stop(jail);
    free(jail);
    jail = next;
  }
}

int main(int argc, char *argv[]) {
  struct sigaction act;
  f2b_config_t config;
  char opt = '\0';

  while ((opt = getopt(argc, argv, "c:dht")) != -1) {
    switch (opt) {
      case 'c':
        strlcpy(appconfig.config_path, optarg, sizeof(appconfig.config_path));
        break;
      case 'd':
        appconfig.daemon = true;
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
  SA_REGISTER(SIGUSR1, &signal_handler);

  if (appconfig.config_path[0] == '\0')
    usage(EXIT_FAILURE);
  memset(&config, 0x0, sizeof(config));
  if (f2b_config_load(&config, appconfig.config_path, true) != true) {
    f2b_log_msg(log_error, "can't load config from '%s'", appconfig.config_path);
    return EXIT_FAILURE;
  }
  if (state == test) {
    fprintf(stderr, "config test ok\n");
    exit(EXIT_SUCCESS);
  }
  f2b_appconfig_update(config.main);

  if (appconfig.daemon) {
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
        (setuid(appconfig.uid) != 0 ||
         setgid(appconfig.gid) != 0)) {
      perror("child: setuid()/setgid() failed");
      exit(EXIT_FAILURE);
    }
    if (chdir("/") != 0) {
      perror("child: chdir('/') failed");
      exit(EXIT_FAILURE);
    }
    if (freopen("/dev/null", "r", stdin)  == NULL ||
        freopen("/dev/null", "w", stdout) == NULL ||
        freopen("/dev/null", "w", stderr) == NULL) {
      perror("child: freopen() on std streams failed");
      exit(EXIT_FAILURE);
    }
  }

  if (appconfig.pidfile_path[0] != '\0') {
    FILE *pidfile = NULL;
    if ((pidfile = fopen(appconfig.pidfile_path, "w")) != NULL) {
      if (flock(fileno(pidfile), LOCK_EX | LOCK_NB) != 0) {
        const char *err = (errno == EWOULDBLOCK)
          ? "another instance already running"
          : strerror(errno);
        f2b_log_msg(log_fatal, "can't lock pidfile: %s", err);
        exit(EXIT_FAILURE);
      }
      fprintf(pidfile, "%d\n", getpid());
      fflush(pidfile);
    } else {
      f2b_log_msg(log_warn, "can't open pidfile: %s", strerror(errno));
    }
  }

  if (appconfig.statedir_path[0] != '\0') {
    struct stat st;
    if (stat(appconfig.statedir_path, &st) < 0) {
      if (errno == ENOENT) {
        mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP; /* 0750 */
        mkdir(appconfig.statedir_path, mode);
      } else {
        f2b_log_msg(log_error, "statedir not exists or unaccessible: %s", strerror(errno));
      }
    } else if (!S_ISDIR(st.st_mode)) {
      f2b_log_msg(log_error, "statedir not a directory: %s", strerror(errno));
    }
  }

  if (appconfig.csocket_path[0] != '\0')
    appconfig.csock = f2b_csocket_create(appconfig.csocket_path);

  if (config.defaults)
    f2b_jail_set_defaults(config.defaults);

  jails_start(&config);
  f2b_config_free(&config);

  if (!jails) {
    f2b_log_msg(log_fatal, "no jails configured, exiting");
    return EXIT_FAILURE;
  }

  while (state) {
    for (f2b_jail_t *jail = jails; jail != NULL; jail = jail->next) {
      f2b_jail_process(jail);
    }
    f2b_csocket_poll(appconfig.csock, f2b_cmsg_process);
    sleep(1);
    if (state == logrotate && strcmp(appconfig.logdest, "file") == 0) {
      state = run;
      f2b_log_to_file(appconfig.logfile_path);
    }
    if (state == reconfig) {
      state = run;
      memset(&config, 0x0, sizeof(config));
      if (f2b_config_load(&config, appconfig.config_path, true)) {
        jails_stop(jails);
        jails = NULL;
        if (config.defaults)
          f2b_jail_set_defaults(config.defaults);
        jails_start(&config);
      } else {
        f2b_log_msg(log_error, "can't load config from '%s'", appconfig.config_path);
      }
      f2b_config_free(&config);
    }
  }

  f2b_csocket_destroy(appconfig.csock, appconfig.csocket_path);

  jails_stop(jails);
  jails = NULL;

  return EXIT_SUCCESS;
}
