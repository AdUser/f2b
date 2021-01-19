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
#include "buf.h"
#include "commands.h"
#include "csocket.h"

#include <getopt.h>
#include <signal.h>

/**
 * @def SA_REGISTER
 * Register signal handler
 */
#define SA_REGISTER(SIGNUM, HANDLER) \
  memset(&act, 0x0, sizeof(act)); \
  act.sa_handler = HANDLER; \
  if (sigaction(SIGNUM, &act, NULL) != 0) { \
    f2b_log_msg(log_fatal, "can't register handler for " #SIGNUM); \
    return EXIT_FAILURE; \
  }

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

static f2b_csock_t *csock = NULL;
#ifndef WITH_CSOCKET
/* add stubs to reduce #ifdef count */
f2b_csock_t *
f2b_csocket_create (const char *path) {
  UNUSED(path);
  f2b_log_msg(log_warn, "control socket support was disabled at compile-time");
  return NULL;
}
void
f2b_csocket_destroy(f2b_csock_t *csock) {
  UNUSED(csock); return;
}
int f2b_csocket_poll(f2b_csock_t *csock, void (*cb)(const f2b_cmd_t *cmd, f2b_buf_t *res)) {
  UNUSED(csock); UNUSED(cb); return 0;
}
void
f2b_csocket_cmd_process(const f2b_cmd_t *cmd, f2b_buf_t *res) {
  UNUSED(cmd); UNUSED(res); return;
}
#else /* WITH_CSOCKET */
void
f2b_csocket_cmd_process(const f2b_cmd_t *cmd, f2b_buf_t *res) {
  f2b_jail_t *jail = NULL;
  char buf[4096] = "";
  size_t len;

  assert(cmd != NULL);
  assert(res != NULL);

  if (cmd->type == CMD_UNKNOWN)
    return;

  if (cmd->type == CMD_HELP) {
    f2b_buf_append(res, f2b_cmd_help(), 0);
    return;
  }

  if (cmd->type >= CMD_JAIL_STATUS && cmd->type <= CMD_JAIL_FILTER_RELOAD) {
    if ((jail = f2b_jail_find(jails, cmd->args[1])) == NULL) {
      len = snprintf(buf, sizeof(buf), "can't find jail '%s'\n", cmd->args[1]);
      f2b_buf_append(res, buf, len);
      return;
    }
  }

  if (cmd->type == CMD_RELOAD) {
    state = reconfig;
  } else if (cmd->type == CMD_LOG_ROTATE) {
    state = logrotate;
  } else if (cmd->type == CMD_LOG_LEVEL) {
    f2b_log_set_level(cmd->args[2]);
  } else if (cmd->type == CMD_SHUTDOWN) {
    state = stop;
  } else if (cmd->type == CMD_STATUS) {
    len = snprintf(buf, sizeof(buf), "pid: %u\npidfile: %s\n", getpid(), appconfig.pidfile_path);
    f2b_buf_append(res, buf, len);
    len = snprintf(buf, sizeof(buf), "csocket: %s\n",  appconfig.csocket_path);
    f2b_buf_append(res, buf, len);
    len = snprintf(buf, sizeof(buf), "statedir: %s\n", appconfig.statedir_path);
    f2b_buf_append(res, buf, len);
    f2b_buf_append(res, "jails:\n", 0);
    for (jail = jails; jail != NULL; jail = jail->next) {
      len = snprintf(buf, sizeof(buf), "- %s\n", jail->name);
      f2b_buf_append(res, buf, len);
    }
  } else if (cmd->type == CMD_JAIL_STATUS) {
    f2b_jail_cmd_status(buf, sizeof(buf), jail);
    f2b_buf_append(res, buf, 0);
  } else if (cmd->type == CMD_JAIL_SET) {
    f2b_jail_cmd_set(buf, sizeof(buf), jail, cmd->args[3], cmd->args[4]);
    f2b_buf_append(res, buf, 0);
  } else if (cmd->type == CMD_JAIL_IP_STATUS) {
    f2b_jail_cmd_ip_xxx(buf, sizeof(buf), jail,  0, cmd->args[4]);
    f2b_buf_append(res, buf, 0);
  } else if (cmd->type == CMD_JAIL_IP_BAN) {
    f2b_jail_cmd_ip_xxx(buf, sizeof(buf), jail,  1, cmd->args[4]);
    f2b_buf_append(res, buf, 0);
  } else if (cmd->type == CMD_JAIL_IP_RELEASE) {
    f2b_jail_cmd_ip_xxx(buf, sizeof(buf), jail, -1, cmd->args[4]);
    f2b_buf_append(res, buf, 0);
  } else if (cmd->type == CMD_JAIL_FILTER_STATS) {
    f2b_filter_cmd_stats(buf, sizeof(buf), jail->filter);
    f2b_buf_append(res, buf, 0);
  } else if (cmd->type == CMD_JAIL_FILTER_RELOAD) {
    f2b_filter_cmd_reload(buf, sizeof(buf), jail->filter);
    f2b_buf_append(res, buf, 0);
  } else {
    f2b_buf_append(res, "error: unknown command\n", 0);
  }

  if (res->used == 0)
    f2b_buf_append(res, "ok\n", 3); /* default reply if not set above */

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

  if (appconfig.csocket_path[0] != '\0') {
    csock = f2b_csocket_create(appconfig.csocket_path);
  }

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
    f2b_csocket_poll(csock, f2b_csocket_cmd_process);
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

  f2b_csocket_destroy(csock);

  jails_stop(jails);
  jails = NULL;

  return EXIT_SUCCESS;
}
