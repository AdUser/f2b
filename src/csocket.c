/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "buf.h"
#include "log.h"
#include "commands.h"
#include "csocket.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

typedef struct f2b_conn_t {
  int sock;
  const char *path;
  f2b_buf_t recv;
  f2b_buf_t send;
} f2b_conn_t;

struct f2b_csock_t {
  f2b_conn_t *clients[MAXCONNS];
  const char *path;
  int sock;
  bool shutdown;
};

f2b_csock_t *
f2b_csocket_create(const char *path) {
  f2b_csock_t *csock;
  struct sockaddr_un addr;
  int sock = -1;

  assert(path != NULL);

  if ((sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
    f2b_log_msg(log_error, "can't create control socket: %s", strerror(errno));
    return NULL;
  }

  memset(&addr, 0x0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strlcpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  unlink(path);
  if (bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) != 0) {
    f2b_log_msg(log_error, "bind() on socket failed: %s", strerror(errno));
    return NULL;
  }

  if (listen(sock, 5) < 0) {
    f2b_log_msg(log_error, "listen() on socket failed: %s", strerror(errno));
    return NULL;
  }

  if ((csock = calloc(1, sizeof(f2b_csock_t))) == NULL) {
    f2b_log_msg(log_error, "can't allocate memory for csocket struct");
    shutdown(sock, SHUT_RDWR);
    unlink(path);
    return NULL;
  }

  csock->sock = sock;
  csock->path = path;
  return csock;
}

void
f2b_csocket_destroy(f2b_csock_t *csock) {
  f2b_conn_t *conn = NULL;
  assert(csock != NULL);

  if (csock->sock >= 0)
    shutdown(csock->sock, SHUT_RDWR);
  if (csock->path != NULL)
    unlink(csock->path);
  for (int i = 0; i < MAXCONNS; i++) {
    if ((conn = csock->clients[i]) == NULL)
      continue;
    shutdown(conn->sock, SHUT_RDWR);
    f2b_buf_free(&conn->recv);
    f2b_buf_free(&conn->send);
    free(conn);
  }
  free(csock);

  return;
}

int
f2b_csocket_poll(f2b_csock_t *csock, void (*cb)(const f2b_cmd_t *cmd, f2b_buf_t *res)) {
  f2b_cmd_t *cmd = NULL;
  int ret, processed = 0;

  assert(csock != NULL);
  assert(cb != NULL);

  return processed;
}
