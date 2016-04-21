/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "cmsg.h"
#include "commands.h"
#include "csocket.h"
#include "client.h"
#include "log.h"

#include <signal.h>

struct {
  enum { interactive = 0, oneshot } mode;
  int  csocket;
  char csocket_spath[PATH_MAX];
  char csocket_cpath[PATH_MAX];
} opts = {
  interactive, -1,
  DEFAULT_CSOCKET_PATH,
  DEFAULT_CSOCKET_CPATH, /* template */
};

void usage(int exitcode) {
  fputs("Usage: f2bc [-h] [-s <path>] [-c <command>]\n", stdout);
  fputs("\t-h         Show this message\n", stdout);
  fputs("\t-c <cmd>   Send command to daemon\n", stdout);
  fputs("\t-s <path>  Path to socket (default: " DEFAULT_CSOCKET_PATH ")\n", stdout);
  exit(exitcode);
}

int
handle_cmd(const char *line) {
  f2b_cmsg_t cmsg;
  struct sockaddr_storage addr;
  socklen_t addrlen = 0;
  int ret;

  memset(&addr, 0x0, sizeof(addr));
  memset(&cmsg, 0x0, sizeof(cmsg));

  cmsg.type = f2b_cmd_parse(line, &cmsg.data[0], sizeof(cmsg.data));
  if (cmsg.type == CMD_HELP) {
    f2b_cmd_help();
    return EXIT_SUCCESS;
  } else if (cmsg.type == CMD_NONE) {
    f2b_log_msg(log_warn, "unable to parse command line");
    return EXIT_FAILURE;
  }
  /* fill other fields */
  strncpy(cmsg.magic, "F2B", sizeof(cmsg.magic));
  cmsg.version = F2B_PROTO_VER;
  cmsg.size = strlen(cmsg.data);
  cmsg.data[cmsg.size] = '\0';
  f2b_cmsg_convert_args(&cmsg);
  cmsg.flags |= CMSG_FLAG_NEED_REPLY;
  if ((ret = f2b_csocket_send(opts.csocket, &cmsg, NULL,  &addrlen)) < 0)
    return EXIT_FAILURE;

  memset(&cmsg, 0x0, sizeof(cmsg));
  addrlen = sizeof(addr);
  if ((ret = f2b_csocket_recv(opts.csocket, &cmsg, &addr, &addrlen)) < 0)
    return EXIT_FAILURE;

  if (cmsg.type != CMD_RESP) {
    f2b_log_msg(log_error, "recieved message not a 'response' type");
    return EXIT_FAILURE;
  }
  fputs(cmsg.data, stdout);
  fputc('\n', stdout);
  return EXIT_SUCCESS;
}

void
signal_handler(int signum) {
  switch (signum) {
    case SIGINT:
    case SIGTERM:
      unlink(opts.csocket_cpath);
      break;
    default:
      break;
  }
}

int main(int argc, char *argv[]) {
  struct sigaction act;
  char line[INPUT_LINE_MAX] = "";
  char opt = '\0';
  int ret;

  while ((opt = getopt(argc, argv, "c:hs:")) != -1) {
    switch (opt) {
      case 'c':
        opts.mode = oneshot;
        strlcpy(line, optarg, sizeof(line));
        break;
      case 's':
        strlcpy(opts.csocket_spath, optarg, sizeof(opts.csocket_spath));
        break;
      case 'h':
        usage(EXIT_SUCCESS);
        break;
      default:
        usage(EXIT_FAILURE);
        break;
    }
  }

  SA_REGISTER(SIGTERM, &signal_handler);
  SA_REGISTER(SIGINT,  &signal_handler);

  /* prepare client side of socket */
  mkstemp(opts.csocket_cpath);
  unlink(opts.csocket_cpath); /* remove regular file created by mkstemp() */

  if ((opts.csocket = f2b_csocket_connect(opts.csocket_spath, opts.csocket_cpath)) <= 0)
    exit(EXIT_FAILURE);

  if (opts.mode == oneshot) {
    ret = handle_cmd(line);
    f2b_csocket_disconnect(opts.csocket, opts.csocket_cpath);
    exit(ret);
  }

  while (1) {
    fputs("f2b >> ", stdout);
    fgets(line, sizeof(line) - 1, stdin);
    if (feof(stdin)) {
      fputc('\n', stdout);
      break;
    }
    handle_cmd(line);
  }

  f2b_csocket_disconnect(opts.csocket, opts.csocket_cpath);

  return EXIT_SUCCESS;
}
