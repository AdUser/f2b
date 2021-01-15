/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include "common.h"
#include "client.h"

struct {
  enum { interactive = 0, oneshot } mode;
  char spath[PATH_MAX];
} opts = {
  interactive,
  DEFAULT_CSOCKET_PATH,
};

static int csock = -1;

void usage(int exitcode) {
  fputs("Usage: f2bc [-h] [-s <path>] [-c <command>]\n", stdout);
  fputs("\t-h         Show this message\n", stdout);
  fputs("\t-c <cmd>   Send command to daemon\n", stdout);
  fputs("\t-s <path>  Path to socket (default: " DEFAULT_CSOCKET_PATH ")\n", stdout);
  exit(exitcode);
}

int
handle_recv() {
  char buf[WBUF_SIZE] = ""; /* our "read" is server "write" */
  int ret;

  if (csock < 0)
    return 0; /* not connected */

  ret = recv(csock, &buf, sizeof(buf), MSG_DONTWAIT);
  if (ret > 0) {
    write(fileno(stdout), buf, ret);
  } else if (ret == 0) {
    puts("connection closed");
    exit(EXIT_SUCCESS); /* received EOF */
  } else if (ret < 0 && errno == EAGAIN) {
    return 0;
  } else /* ret < 0 */ {
    perror("recv()");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int
handle_cmd(const char *line) {
  const char *p = NULL;
  char buf[WBUF_SIZE] = ""; /* our "read" is server "write" */
  int ret; int len;

  assert(line != NULL);

  snprintf(buf, sizeof(buf), "%s\n", line);
  p = buf;
  while (p && *p != '\0') {
    len = strlen(p);
    ret = send(csock, p, strlen(p), 0);
    if (ret < 0 && errno == EAGAIN) {
      continue; /* try again */
    } else if (ret < 0) {
      perror("send()");
      exit(EXIT_FAILURE);
    } else if (ret == len){
      break; /* all data sent */
    } else /* ret > 0 */ {
      p += ret;
    }
  }

  return 0;
}

void
handle_signal(int signum) {
  switch (signum) {
    case SIGINT:
    case SIGTERM:
      exit(EXIT_SUCCESS);
      break;
    default:
      break;
  }
}

void
setup_sigaction(int signum) {
  struct sigaction act;
  memset(&act, 0x0, sizeof(act));
  act.sa_handler = &handle_signal;
  if (sigaction(signum, &act, NULL) != 0) {
    perror("sigaction()");
    exit(EXIT_FAILURE);
  }
}

void
setup_socket() {
  struct sockaddr_un saddr;

  if ((csock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  memset(&saddr, 0x0, sizeof(saddr));
  saddr.sun_family = AF_UNIX;
  strlcpy(saddr.sun_path, opts.spath, sizeof(saddr.sun_path) - 1);
  if (connect(csock, (struct sockaddr *) &saddr, sizeof(struct sockaddr_un)) < 0) {
    perror("connect()");
    exit(EXIT_FAILURE);
  }
}

#ifdef WITH_READLINE
  #include <readline/readline.h>
  #include <readline/history.h>
  rl_event_hook = &handle_recv;
#else
char *
readline(const char *prompt) {
  char line[RBUF_SIZE+1];
  char *p;

  while (1) {
    line[0] = '\0';
    p = &line[0];
    handle_recv();
    fputs(prompt, stdout);
    if (!fgets(line, sizeof(line) - 1, stdin)) {
      if (!feof(stdin))
        fputs("read error\n", stdout);
      return NULL;
    }
    while (isspace(*p)) p++;
    if (*p != '\n' && *p != '\0')
      return strdup(p);
  }
  return NULL;
}

/* stubs */
#define add_history(x) (void)(x);
#define using_history()
#endif

int main(int argc, char *argv[]) {
  char *line = NULL;
  char opt = '\0';
  int ret;

  while ((opt = getopt(argc, argv, "c:hs:")) != -1) {
    switch (opt) {
      case 'c':
        opts.mode = oneshot;
        line = strndup(optarg, RBUF_SIZE);
        break;
      case 's':
        strlcpy(opts.spath, optarg, sizeof(opts.spath));
        break;
      case 'h':
        usage(EXIT_SUCCESS);
        break;
      default:
        usage(EXIT_FAILURE);
        break;
    }
  }

  setup_sigaction(SIGTERM);
  setup_sigaction(SIGINT);

  setup_socket();

  if (opts.mode == oneshot) {
    ret = handle_cmd(line);
    shutdown(csock, SHUT_RDWR);
    exit(ret);
  }

  using_history();
  while ((line = readline("f2b >> ")) != NULL) {
    if (line[0] != '\n' && line[0] != '\0') {
      add_history(line);
      handle_cmd(line);
    }
    free(line);
    line = NULL;
  }
  putc('\n', stdout);

  return EXIT_SUCCESS;
}
