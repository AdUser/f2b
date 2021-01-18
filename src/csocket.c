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
#include <sys/select.h>

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

/* helpers */

static inline int
max(int a, int b) {
  return a > b ? a : b;
}

/* connection-related functions */

f2b_conn_t *
f2b_conn_create(size_t rbuf, size_t wbuf) {
  f2b_conn_t *conn = NULL;
  if ((conn = calloc(1, sizeof(f2b_conn_t))) != NULL) {
    if (f2b_buf_alloc(&conn->recv, rbuf)) {
      if (f2b_buf_alloc(&conn->send, wbuf)) {
        return conn;
      }
      f2b_buf_free(&conn->recv);
    }
    free(conn);
  }
  return NULL;
}

void
f2b_conn_destroy(f2b_conn_t *conn) {
  f2b_buf_free(&conn->recv);
  f2b_buf_free(&conn->send);
  free(conn);
}

/* control socket-related functions */

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
    f2b_conn_destroy(conn);
    csock->clients[i] = NULL;
  }
  free(csock);

  return;
}

void
f2b_csocket_poll(f2b_csock_t *csock, void (*cb)(const f2b_cmd_t *cmd, f2b_buf_t *res)) {
  struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
  fd_set rfds, wfds;
  f2b_conn_t *conn = NULL;
  f2b_cmd_t *cmd = NULL;
  int retval, nfds;

  assert(csock != NULL);
  assert(cb != NULL);

  /* loop / init */
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);

  FD_SET(csock->sock, &rfds); /* watch for new connections */

  /* watch for new data on established connections */
  for (int cnum = 0; cnum < MAXCONNS; cnum++) {
    if ((conn = csock->clients[cnum]) == NULL)
      continue;
    if (!csock->shutdown)
      FD_SET(conn->sock, &rfds);
    if (conn->send.used)
      FD_SET(conn->sock, &wfds);
    nfds = max(csock->sock, conn->sock);
  }

  /* check for new data */
  retval = select(nfds + 1, &rfds, &wfds, NULL, &tv);
  if (retval < 0) {
    if (errno == EINTR) {
      /* interrupted by signal */
    } else if (errno == EAGAIN) {
      /* no data */
    } else {
      f2b_log_msg(log_error, "select() error: %s", strerror(errno));
    }
    return;
  }
  if (retval == 0)
    return; /* no new data */

  /* new connection on listening socket? */
  if (FD_ISSET(csock->sock, &rfds)) {
    /* find free connection slot */
    int cnum = 0;
    for (int cnum = 0; cnum < MAXCONNS; cnum++) {
      if (csock->clients[cnum] == NULL) break;
    }
    int sock = -1;
    /* accept() new connection */
    if ((sock = accept(csock->sock, NULL, NULL)) < 0) {
      perror("accept()");
    } else if (cnum < MAXCONNS) {
      if ((conn = f2b_conn_create(RBUF_SIZE, WBUF_SIZE)) != NULL) {
        f2b_log_msg(log_debug, "new connection accept()ed, socket %d, conn %d\n", sock, cnum);
        conn->sock = sock;
        csock->clients[cnum] = conn;
      } else {
        f2b_log_msg(log_error, "can;t create new connection");
      }
    } else {
      f2b_log_msg(log_error, "max number of clients reached, drop connection on socket %d\n", sock);
      shutdown(sock, SHUT_RDWR);
    }
  }

  for (int cnum = 0; cnum < MAXCONNS; cnum++) {
    if ((conn = csock->clients[cnum]) == NULL)
      continue;
    /* handle incoming data */
    if (FD_ISSET(conn->sock, &rfds)) {
      f2b_log_msg(log_debug, "some incoming data on socket %d", conn->sock);
      char tmp[RBUF_SIZE] = "";
      char *line = NULL;
      ssize_t read = 0;
      read = recv(conn->sock, tmp, RBUF_SIZE, MSG_DONTWAIT);
      if (read > 0) {
        tmp[read] = '\0';
        f2b_log_msg(log_debug, "conn %d received %zd bytes, and recv buf is now %zd bytes\n", conn->sock, read, conn->recv.used);
        f2b_buf_append(&conn->recv, tmp, read);
        /* TODO: properly handle empty lines */
        while (*conn->recv.data == '\n') {
          /* TODO f2b_buf_splice(conn->send, retval); */
          break;
        }
        /* extract message(s) */
        while ((line = f2b_buf_extract(&conn->recv, "\n")) != NULL) {
          if ((cmd = f2b_cmd_create(line)) != NULL) {
            cb(cmd, &conn->send); /* handle command */
            f2b_cmd_destroy(cmd);
          } else {
            f2b_buf_append(&conn->send, "can't parse input\n", 0);
          }
          free(line);
        }
      } else if (read == 0) {
        f2b_log_msg(log_debug, "received connection close on socket %d", conn->sock);
        shutdown(conn->sock, SHUT_RDWR);
        f2b_conn_destroy(conn);
        csock->clients[cnum] = NULL;
      } else {
        perror("recv()");
      }
    }
    /* handle outgoing data */
    if (conn->send.used > 0) {
      f2b_log_msg(log_debug, "sending %zu bytes to socket %d\n", conn->send.used, conn->sock);
      retval = send(conn->sock, conn->send.data, conn->send.used, MSG_DONTWAIT);
      if (retval > 0) {
        /* TODO f2b_buf_splice(conn->send, retval); */
      } else if (retval < 0) {
        f2b_log_msg(log_error, "can't send %zu bytes to socket %d", conn->send.used, conn->sock);
      }
    }
  } /* foreach connection(s) */
  return;
}
