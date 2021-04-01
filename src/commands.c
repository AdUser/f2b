/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "buf.h"
#include "commands.h"

struct cmd_desc {
  enum f2b_command_type type;
  const short int argc;
  const short int tokenc;
  const char *help;
  const char *tokens[CMD_TOKENS_MAXCOUNT];
} commands[] = {
  {
    .type = CMD_HELP,
    .argc = 0, .tokenc = 1,
    .tokens = { "help", NULL },
    .help = "Show available commands",
  }, {
    .type = CMD_AUTH,
    .argc = 2, .tokenc = 3,
    .tokens = { "auth", "<type>", "<password>", NULL },
    .help = "Check authorization (available types is: plain, challenge)",
  }, {
    .type = CMD_STATUS,
    .argc = 0, .tokenc = 1,
    .tokens = { "status", NULL },
    .help = "Show general stats and jails list",
  }, {
    .type = CMD_RELOAD,
    .argc = 0, .tokenc = 1,
    .tokens = { "reload", NULL },
    .help = "Reload own config, all jails will be reset.",
  }, {
    .type = CMD_SHUTDOWN,
    .argc = 0, .tokenc = 1,
    .tokens = { "shutdown", NULL },
    .help = "Gracefully terminate f2b daemon",
  }, {
    .type = CMD_LOG_ROTATE,
    .argc = 0, .tokenc = 2,
    .tokens = { "log", "rotate", NULL },
    .help = "Reopen daemon's own log file",
  }, {
    .type = CMD_LOG_LEVEL,
    .argc = 1, .tokenc = 3,
    .tokens = { "log", "level", "<level>",  NULL },
    .help = "Change maximum level of logged messages",
  }, {
    .type = CMD_LOG_EVENTS,
    .argc = 1, .tokenc = 3,
    .tokens = { "log", "events", "<bool>",  NULL },
    .help = "Enable/disable sending events to connected client",
  }, {
    .type = CMD_JAIL_STATUS,
    .argc = 1, .tokenc = 3,
    .tokens = { "jail", "<jailname>", "status", NULL },
    .help = "Show status and stats of given jail",
  }, {
    .type = CMD_JAIL_SET,
    .argc = 3, .tokenc = 5,
    .tokens = { "jail", "<jailname>", "set", "<param>", "<value>", NULL },
    .help = "Set parameter of given jail",
  }, {
    .type = CMD_JAIL_IP_STATUS,
    .argc = 2, .tokenc = 5,
    .tokens = { "jail", "<jailname>", "ip", "status", "<ip>", NULL },
    .help = "Show ip status in given jail",
  }, {
    .type = CMD_JAIL_IP_BAN,
    .argc = 2, .tokenc = 5,
    .tokens = { "jail", "<jailname>", "ip", "ban", "<ip>", NULL },
    .help = "Forcefully ban some ip in given jail",
  }, {
    .type = CMD_JAIL_IP_RELEASE,
    .argc = 2, .tokenc = 5,
    .tokens = { "jail", "<jailname>", "ip", "release", "<ip>", NULL },
    .help = "Forcefully release some ip in given jail",
  }, {
    .type = CMD_JAIL_SOURCE_STATS,
    .argc = 1, .tokenc = 4,
    .tokens = { "jail", "<jailname>", "source", "stats", NULL },
    .help = "Show source stats for jail",
  }, {
    .type = CMD_JAIL_FILTER_STATS,
    .argc = 1, .tokenc = 4,
    .tokens = { "jail", "<jailname>", "filter", "stats", NULL },
    .help = "Show matches stats for jail regexps",
  }, {
    .type = CMD_JAIL_FILTER_RELOAD,
    .argc = 1, .tokenc = 4,
    .tokens = { "jail", "<jailname>", "filter", "reload", NULL },
    .help = "Reload regexps for given jail",
  }, {
    .type = CMD_UNKNOWN,
    .argc = 0, .tokenc = 0,
    .tokens = { NULL },
    .help = "",
  }
};

static char *help = NULL;

const char *
f2b_cmd_help() {
  f2b_buf_t buf;
  const char **p = NULL;

  if (help)
    return help;

  if (!f2b_buf_alloc(&buf, 8192))
    return "internal error: can't allocate memory\n";
  f2b_buf_append(&buf, "Available commands:\n\n", 0);
  for (struct cmd_desc *cmd = commands; cmd->type != CMD_UNKNOWN; cmd++) {
    for (p = cmd->tokens; *p != NULL; p++) {
      f2b_buf_append(&buf, *p, 0);
      f2b_buf_append(&buf, " ", 1);
    }
    f2b_buf_append(&buf, "\n\t", 2);
    f2b_buf_append(&buf, cmd->help, 0);
    f2b_buf_append(&buf, "\n\n", 2);
  }

  help = strndup(buf.data, buf.used);
  f2b_buf_free(&buf);
  return help;
}

f2b_cmd_t *
f2b_cmd_create(const char *line) {
  f2b_cmd_t *cmd = NULL;

  assert(line != NULL);

  while (isspace(*line)) line++;
  if (strlen(line) <= 0)
    return NULL; /* empty string */

  if ((cmd = calloc(1, sizeof(f2b_cmd_t))) == NULL)
    return NULL;

  if (f2b_cmd_parse(cmd, line))
    return cmd;
  free(cmd);
  return NULL;
}

void
f2b_cmd_destroy(f2b_cmd_t *cmd) {
  if (!cmd) return;
  f2b_buf_free(&cmd->data);
  free(cmd);
}

bool
f2b_cmd_parse(f2b_cmd_t *cmd, const char *src) {
  char *p = NULL;

  assert(cmd != NULL);
  assert(src != NULL);

  /* strip leading spaces */
  while (isblank(*src))
    src++;

  if (strlen(src) == 0)
    return false; /* empty string */

  f2b_buf_alloc(&cmd->data, strlen(src) + 1);
  f2b_buf_append(&cmd->data, src, 0);

  /* simple commands without args */
  cmd->argc = 1; /* we has at least one arg */
  cmd->args[0] = cmd->data.data;
       if (strcmp(src, "help")     == 0) { cmd->type = CMD_HELP;     return true; }
  else if (strcmp(src, "status")   == 0) { cmd->type = CMD_STATUS;   return true; }
  else if (strcmp(src, "reload")   == 0) { cmd->type = CMD_RELOAD;   return true; }
  else if (strcmp(src, "shutdown") == 0) { cmd->type = CMD_SHUTDOWN; return true; }

  /* split string to tokens */
  p = cmd->data.data;
  cmd->args[0] = strtok_r(cmd->data.data, " \t", &p);
  for (size_t i = 1; i < CMD_TOKENS_MAXCOUNT; i++) {
    cmd->args[i] = strtok_r(NULL, " \t", &p);
    if (cmd->args[i] == NULL)
      break;
    cmd->argc++;
  }

  if (strcmp(cmd->args[0], "jail") == 0 && cmd->argc > 1) {
    /* commands for jail */
    if (cmd->argc == 3 && strcmp(cmd->args[2], "status") == 0) {
      cmd->type = CMD_JAIL_STATUS; return true;
    }
    if (cmd->argc == 5 && strcmp(cmd->args[2], "set") == 0) {
      cmd->type = CMD_JAIL_SET; return true;
    }
    if (cmd->argc == 5 && strcmp(cmd->args[2], "ip") == 0 && strcmp(cmd->args[3], "status") == 0) {
      cmd->type = CMD_JAIL_IP_STATUS; return true;
    }
    if (cmd->argc == 5 && strcmp(cmd->args[2], "ip") == 0 && strcmp(cmd->args[3], "ban") == 0) {
      cmd->type = CMD_JAIL_IP_BAN; return true;
    }
    if (cmd->argc == 5 && strcmp(cmd->args[2], "ip") == 0 && strcmp(cmd->args[3], "release") == 0) {
      cmd->type = CMD_JAIL_IP_RELEASE; return true;
    }
    if (cmd->argc == 4 && strcmp(cmd->args[2], "source") == 0 && strcmp(cmd->args[3], "stats") == 0) {
      cmd->type = CMD_JAIL_SOURCE_STATS; return true;
    }
    if (cmd->argc == 4 && strcmp(cmd->args[2], "filter") == 0 && strcmp(cmd->args[3], "stats") == 0) {
      cmd->type = CMD_JAIL_FILTER_STATS; return true;
    }
    if (cmd->argc == 4 && strcmp(cmd->args[2], "filter") == 0 && strcmp(cmd->args[3], "reload") == 0) {
      cmd->type = CMD_JAIL_FILTER_RELOAD; return true;
    }
  } else if (strcmp(cmd->args[0], "auth") == 0 && cmd->argc > 1) {
    if (cmd->argc == 3 && strcmp(cmd->args[1], "plain") == 0) {
      cmd->type = CMD_AUTH; return true;
    }
    if (cmd->argc == 3 && strcmp(cmd->args[1], "challenge") == 0) {
      cmd->type = CMD_AUTH; return true;
    }
  } else if (strcmp(cmd->args[0], "log") == 0 && cmd->argc > 1) {
    if (cmd->argc == 2 && strcmp(cmd->args[1], "rotate") == 0) {
      cmd->type = CMD_LOG_ROTATE; return true;
    }
    if (cmd->argc == 3 && strcmp(cmd->args[1], "level") == 0) {
      cmd->type = CMD_LOG_LEVEL; return true;
    }
    if (cmd->argc == 3 && strcmp(cmd->args[1], "events") == 0) {
      cmd->type = CMD_LOG_EVENTS; return true;
    }
  }
  cmd->type = CMD_UNKNOWN;
  memset(cmd->args, 0x0, sizeof(cmd->args));
  f2b_buf_free(&cmd->data);

  return false;
}
