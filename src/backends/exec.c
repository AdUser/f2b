#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "backend.h"

#define ID_MAX 32

typedef struct cmd_t {
  struct cmd_t *next;
  char  *args;   /**< stores path of cmd & args, delimited by '\0' */
  char **argv;   /**< stores array of pointers to args + NULL */
  size_t argc;   /**< args count */
  size_t pos;    /**< index in argv[] where to insert IP address */
} cmd_t;

struct _config {
  char name[ID_MAX + 1];
  cmd_t *start;
  cmd_t *stop;
  cmd_t *ban;
  cmd_t *unban;
  cmd_t *exists;
};

static cmd_t *
cmd_from_str(const char *str) {
  cmd_t *cmd = NULL;
  const char *delim = " \t";
  char *token, *saveptr, *argv;

  if ((cmd = calloc(1, sizeof(cmd_t))) == NULL)
    return NULL;

  if ((cmd->args = strdup(str)) == NULL)
    goto cleanup;

  strtok_r(cmd->args, delim, &saveptr);
  while ((token = strtok_r(NULL, delim, &saveptr)) != NULL) {
    if ((argv = realloc(cmd->argv, sizeof(cmd->argv) * (cmd->argc + 2))) == NULL)
      goto cleanup;
   *cmd->argv = argv;
    if (strcmp(token, HOST_TOKEN) == 0)
      cmd->pos = cmd->argc;
    cmd->argv[cmd->argc] = token;
    cmd->argc++;
  }
  cmd->argv[cmd->argc + 1] = NULL;

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

  CREATE_CMD(start)
  CREATE_CMD(stop)
  CREATE_CMD(ban)
  CREATE_CMD(unban)
  CREATE_CMD(exists)

  return false;
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
  cmd_list_destroy(cfg->exists);
  free(cfg);
}
