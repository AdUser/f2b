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
  f2b_buf_t recv;
  f2b_buf_t send;
  int sock;
} f2b_conn_t;

struct f2b_csock_t {
  f2b_conn_t *clients[MAXCONNS];
  const char *path;
  int sock;
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

int
f2b_conn_process(f2b_conn_t *conn, bool in, void (*cb)(const f2b_cmd_t *cmd, f2b_buf_t *res)) {
  f2b_cmd_t *cmd = NULL;
  int retval;

  /* handle incoming data */
  if (in) {
    char tmp[RBUF_SIZE] = "";
    char *line = NULL;
    ssize_t read = 0;
    size_t avail = conn->recv.size - conn->recv.used;
    read = recv(conn->sock, tmp, avail, MSG_DONTWAIT);
    if (read == 0 || (read < 0 && errno == ECONNRESET)) {
      f2b_log_msg(log_debug, "received connection close on socket %d", conn->sock);
      return -1;
    }
    if (read < 0) {
      f2b_log_msg(log_error, "received error on sock %d: %s", conn->sock, strerror(errno));
      return -1;
    }
    if (read > 0) {
      tmp[read] = '\0';
      for (const char *p = tmp; *p != '\0'; p++) {
        if (isgraph(*p) || isspace(*p))
          continue;
        f2b_log_msg(log_error, "non-printable character in data on sock %d", conn->sock);
        return -1;
      }
      retval = f2b_buf_append(&conn->recv, tmp, read);
      f2b_log_msg(log_debug, "received %zd bytes from socket %d, append %d to buf", read, conn->sock, retval);
      /* extract message(s) */
      while ((line = f2b_buf_extract(&conn->recv, "\n")) != NULL) {
        if (strlen(line) == 0) {
          free(line);
          continue;
        }
        f2b_log_msg(log_debug, "extracted line: %s", line);
        if ((cmd = f2b_cmd_create(line)) != NULL) {
          cb(cmd, &conn->send); /* handle command */
          f2b_cmd_destroy(cmd);
        } else {
          f2b_buf_append(&conn->send, "can't parse input\n", 0);
        }
        free(line);
      }
      if (conn->recv.used >= conn->recv.size) {
        f2b_log_msg(log_error, "drop connection on socket %d, recv buffer overflow", conn->sock);
        return -1;
      }
    }
  }
  /* handle outgoing data */
  if (conn->send.used > 0) {
    f2b_log_msg(log_debug, "sending %zu bytes to socket %d", conn->send.used, conn->sock);
    retval = send(conn->sock, conn->send.data, conn->send.used, MSG_DONTWAIT | MSG_NOSIGNAL);
    if (retval > 0) {
      f2b_buf_splice(&conn->send, retval);
      f2b_log_msg(log_debug, "sent %d bytes to socket %d (%zu remains)", retval, conn->sock, conn->send.used);
    } else if (retval < 0 && errno != EAGAIN) {
      f2b_log_msg(log_error, "can't send() to socket %d: %s", conn->sock, strerror(errno));
      return -1; /* remote side closed connection */
    }
  }
  return 0;
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
  struct ucred peer;
  socklen_t peerlen = 0;
  fd_set rfds, wfds;
  f2b_conn_t *conn = NULL;
  int retval, nfds;

  assert(csock != NULL);
  assert(cb != NULL);

  /* loop / init */
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);

  FD_SET(csock->sock, &rfds); /* watch for new connections */

  /* watch for new data on established connections */
  nfds = csock->sock;
  for (int cnum = 0; cnum < MAXCONNS; cnum++) {
    if ((conn = csock->clients[cnum]) == NULL)
      continue;
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
      f2b_log_msg(log_error, "can't accept() new connection: %s", strerror(errno));
    } else if (cnum < MAXCONNS) {
      peerlen = sizeof(peer);
      if (getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &peer, &peerlen) < 0)
        f2b_log_msg(log_error, "can't get remote peer credentials: %s", strerror(errno));
      if ((conn = f2b_conn_create(RBUF_SIZE, WBUF_SIZE)) != NULL) {
        f2b_log_msg(log_debug, "new connection accept()ed, socket %d from uid %d", sock, peer.uid);
        conn->sock = sock;
        csock->clients[cnum] = conn;
      } else {
        f2b_log_msg(log_error, "can't create new connection");
      }
    } else {
      f2b_log_msg(log_error, "max number of clients reached, drop connection on socket %d", sock);
      shutdown(sock, SHUT_RDWR);
    }
  }

  for (int cnum = 0; cnum < MAXCONNS; cnum++) {
    if ((conn = csock->clients[cnum]) == NULL)
      continue;
    retval = f2b_conn_process(conn, FD_ISSET(conn->sock, &rfds), cb);
    if (retval < 0) {
      f2b_log_msg(log_debug, "closing connection on socket %d", conn->sock);
      shutdown(conn->sock, SHUT_RDWR);
      f2b_conn_destroy(conn);
      csock->clients[cnum] = NULL;
    }
  } /* foreach connection(s) */
  return;
}
