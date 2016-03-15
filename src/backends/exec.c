/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "backend.h"

typedef struct cmd_t {
  struct cmd_t *next;
  char  *args;   /**< stores path of cmd & args, delimited by '\0' */
  char **argv;   /**< stores array of pointers to args + NULL */
  size_t argc;   /**< args count */
  size_t pos_ip; /**< index+1 in argv[] where to insert IP address (zero means "no placeholder") */
  size_t pos_id; /**< index+1 in argv[] where to insert IP address (zero means "no placeholder") */
} cmd_t;

struct _config {
  char name[ID_MAX + 1];
  char error[256];
  time_t timeout;
  cmd_t *start;
  cmd_t *stop;
  cmd_t *ban;
  cmd_t *unban;
  cmd_t *check;
};

static cmd_t *
cmd_from_str(const char *str) {
  cmd_t *cmd = NULL;
  const char *delim = " \t";
  char *token, *saveptr, **argv;

  assert(str != NULL);

  if ((cmd = calloc(1, sizeof(cmd_t))) == NULL)
    return NULL;

  if ((cmd->args = strdup(str)) == NULL)
    goto cleanup;

  cmd->argc = 1;
  if ((cmd->argv = calloc(2, sizeof(cmd->argv))) == NULL)
    goto cleanup;
  cmd->argv[cmd->argc] = NULL;

  strtok_r(cmd->args, delim, &saveptr);
  cmd->argv[0] = cmd->args;

  while ((token = strtok_r(NULL, delim, &saveptr)) != NULL) {
    if ((argv = realloc(cmd->argv, sizeof(cmd->argv) * (cmd->argc + 2))) == NULL)
      goto cleanup;
    cmd->argv = argv;
    if (strcmp(token, TOKEN_ID) == 0)
      cmd->pos_id = cmd->argc;
    if (strcmp(token, TOKEN_IP) == 0)
      cmd->pos_ip = cmd->argc;
    cmd->argv[cmd->argc] = token;
    cmd->argc++;
  }
  cmd->argv[cmd->argc] = NULL;

  return cmd;

  cleanup:
  free(cmd->args);
  free(cmd->argv);
  free(cmd);
  return NULL;
}

static cmd_t *
cmd_list_append(cmd_t *list, cmd_t *cmd) {
  cmd_t *c = NULL;

  assert(cmd != NULL);

  if (!list)
    return cmd;

  for (c = list; c->next != NULL; c = c->next);

  c->next = cmd;
  return list;
}

static void
cmd_list_destroy(cmd_t *list) {
  cmd_t *next = NULL;

  for (; list != NULL; list = next) {
    next = list->next;
    free(list->args);
    free(list->argv);
    free(list);
  }
}

static bool
cmd_list_exec(cfg_t *cfg, cmd_t *list, const char *ip) {
  int status = 0;
  pid_t pid;

  for (cmd_t *cmd = list; cmd != NULL; cmd = cmd->next) {
    pid = fork();
    if (pid == 0) {
      /* child */
      if (cmd->pos_ip && ip)
        cmd->argv[cmd->pos_ip] = strdup(ip);
      if (cmd->pos_id)
        cmd->argv[cmd->pos_id] = strdup(cfg->name);
      if (cfg->timeout)
        alarm(cfg->timeout);
      execv(cmd->args, cmd->argv);
    } else if (pid > 0) {
      /* parent */
      waitpid(pid, &status, 0);
      if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        continue;
      if (WIFEXITED(status)) {
        snprintf(cfg->error, sizeof(cfg->error),
          "cmd '%s' terminated with code %d",
          cmd->args, WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
        snprintf(cfg->error, sizeof(cfg->error),
          "cmd '%s' terminated by signal %d", cmd->args, WTERMSIG(status));
      }
      return false;
    } else {
      /* parent, fork error */
      snprintf(cfg->error, sizeof(cfg->error), "can't fork(): %s", strerror(errno));
      return false;
    }
  }

  return true;
}

/* handlers */
cfg_t *
create(const char *id) {
  cfg_t *cfg = NULL;

  assert(id != NULL);

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  strncpy(cfg->name, id, sizeof(cfg->name));
  cfg->name[ID_MAX] = '\0';

  return cfg;
}

#define CREATE_CMD(TYPE) \
  if (strcmp(key, #TYPE) == 0) { \
    cmd = cmd_from_str(value); \
    if (cmd) { \
      cfg->TYPE = cmd_list_append(cfg->TYPE, cmd); \
      return true; \
    } \
    return false; \
  }

bool
config(cfg_t *cfg, const char *key, const char *value) {
  cmd_t *cmd = NULL;

  assert(cfg != NULL);
  assert(key != NULL);
  assert(value != NULL);

  if (strcmp(key, "timeout") == 0) {
    cfg->timeout = atoi(value);
    return true;
  }

  CREATE_CMD(start)
  CREATE_CMD(stop)
  CREATE_CMD(ban)
  CREATE_CMD(unban)
  CREATE_CMD(check)

  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->ban && cfg->unban)
    return true;

  return false;
}

char *
error(cfg_t *cfg) {
  assert(cfg != NULL);

  return cfg->error;
}

bool
start(cfg_t *cfg) {
  assert(cfg != NULL);

  if (!cfg->start)
    return true;

  return cmd_list_exec(cfg, cfg->start, NULL);
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  if (!cfg->stop)
    return true;

  return cmd_list_exec(cfg, cfg->stop, NULL);
}

bool
ban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL && ip != NULL);

  return cmd_list_exec(cfg, cfg->ban, ip);
}

bool
unban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL && ip != NULL);

  return cmd_list_exec(cfg, cfg->unban, ip);
}

bool
check(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL && ip != NULL);

  if (!cfg->check)
    return false;

  return cmd_list_exec(cfg, cfg->check, ip);
}

bool
ping(cfg_t *cfg) {
  return cfg != NULL;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  /* free commands */
  cmd_list_destroy(cfg->start);
  cmd_list_destroy(cfg->stop);
  cmd_list_destroy(cfg->ban);
  cmd_list_destroy(cfg->unban);
  cmd_list_destroy(cfg->check);
  free(cfg);
}
