/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "cmsg.h"
#include "commands.h"

struct f2b_cmd_t {
  const char *help;
  const char *tokens[CMD_TOKENS_MAX];
  char *data;
} commands[] = {
  [CMD_NONE] = {
    .tokens = { NULL },
    .help = "Unspecified command"
  },
  [CMD_RESP] = {
    .tokens = { NULL },
    .help = "Command response, used internally",
  },
  [CMD_HELP] = {
    .tokens = { "help", NULL },
    .help = "Show available commands",
  },
  [CMD_PING] = {
    .tokens = { "ping", NULL },
    .help = "Check the connection",
  },
  [CMD_STATUS] = {
    .tokens = { "status", NULL },
    .help = "Show general stats and jails list",
  },
  [CMD_ROTATE] = {
    .tokens = { "rotate", NULL },
    .help = "Reopen daemon's own log file",
  },
  [CMD_RELOAD] = {
    .tokens = { "reload", NULL },
    .help = "Reload own config, all jails will be reset.",
  },
  [CMD_SHUTDOWN] = {
    .tokens = { "shutdown", NULL },
    .help = "Gracefully terminate f2b daemon",
  },
  [CMD_JAIL_STATUS] = {
    .tokens = { "jail", "<jailname>", "status", NULL },
    .help = "Show status and stats of given jail",
  },
  [CMD_JAIL_SET] = {
    .tokens = { "jail", "<jailname>", "set", "<param>", "<value>", NULL },
    .help = "Set parameter of given jail",
  },
  [CMD_JAIL_IP_STATUS] = {
    .tokens = { "jail", "<jailname>", "status", "<ip>", NULL },
    .help = "Show ip status in given jail",
  },
  [CMD_JAIL_IP_BAN] = {
    .tokens = { "jail", "<jailname>", "ban", "<ip>", NULL },
    .help = "Forcefully ban some ip in given jail",
  },
  [CMD_JAIL_IP_RELEASE] = {
    .tokens = { "jail", "<jailname>", "release", "<ip>", NULL },
    .help = "Forcefully release some ip in given jail",
  },
};

void
f2b_cmd_help() {
  struct f2b_cmd_t *cmd = NULL;
  const char **p = NULL;

  fputs("Available commands:\n\n", stdout);
  for (size_t i = CMD_PING; i < CMD_MAX_NUMBER; i++) {
    cmd = &commands[i];
    if (cmd->tokens[0] == NULL)
      continue;
    for (p = cmd->tokens; *p != NULL; p++)
      fprintf(stdout, "%s ", *p);
    fprintf(stdout, "\n\t%s\n\n", cmd->help);
  }

  return;
}

/**
 * @brief Parse command from line
 * @param src  Line taken from user input
 * @param buf  Buffer for command parameters
 * @param buflen Size of buffer above
 * @return Type of parsed command or CMD_NONE if no matches
 */
enum f2b_cmsg_type
f2b_cmd_parse(const char *src, char *buf, size_t buflen) {
  size_t tokenc = 0; /* tokens count */
  char *tokens[CMD_TOKENS_MAX] = { NULL };
  char line[INPUT_LINE_MAX];
  char *p;

  assert(line != NULL);
  /* strip leading spaces */
  while (isblank(*line))
    src++;
  /* strip trailing spaces, newlines, etc */
  strlcpy(line, src, sizeof(line));
  for (size_t l = strlen(line); l >= 1 && isspace(line[l - 1]); l--)
    line[l - 1] = '\0';

  if (line[0] == '\0')
    return CMD_NONE; /* empty string */
  tokenc = 1;

  buf[0] = '\0';

  /* split string to tokens */
  tokens[0] = strtok_r(line, " \t", &p);
  for (size_t i = 1; i < CMD_TOKENS_MAX; i++) {
    tokens[i] = strtok_r(NULL, " \t", &p);
    if (tokens[i] == NULL)
      break;
    tokenc++;
  }

  if      (strcmp(line, "ping")     == 0) { return CMD_PING;     }
  else if (strcmp(line, "help")     == 0) { return CMD_HELP;     }
  else if (strcmp(line, "status")   == 0) { return CMD_STATUS;   }
  else if (strcmp(line, "rotate")   == 0) { return CMD_ROTATE;   }
  else if (strcmp(line, "reload")   == 0) { return CMD_RELOAD;   }
  else if (strcmp(line, "shutdown") == 0) { return CMD_SHUTDOWN; }
  else if (strcmp(line, "jail") == 0 && tokenc > 1) {
    /* commands for jail */
    strlcpy(buf, tokens[1], buflen);
    strlcat(buf, "\n", buflen);
    if (tokenc == 3 && strcmp(tokens[2], "status") == 0) {
      return CMD_JAIL_STATUS;
    }
    if (tokenc == 5 && strcmp(tokens[2], "set") == 0) {
      strlcat(buf, tokens[3], buflen);
      strlcat(buf, "\n", buflen);
      strlcat(buf, tokens[4], buflen);
      strlcat(buf, "\n", buflen);
      return CMD_JAIL_SET;
    }
    if (tokenc == 4 && strcmp(tokens[2], "status") == 0) {
      strlcat(buf, tokens[3], buflen);
      strlcat(buf, "\n", buflen);
      return CMD_JAIL_IP_STATUS;
    }
    if (tokenc == 4 && strcmp(tokens[2], "ban") == 0) {
      strlcat(buf, tokens[3], buflen);
      strlcat(buf, "\n", buflen);
      return CMD_JAIL_IP_BAN;
    }
    if (tokenc == 4 && strcmp(tokens[2], "release") == 0) {
      strlcat(buf, tokens[3], buflen);
      strlcat(buf, "\n", buflen);
      return CMD_JAIL_IP_RELEASE;
    }
  }

  return CMD_NONE;
}
