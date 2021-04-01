/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "config.h"
#include "buf.h"
#include "log.h"
#include "md5.h"
#include "commands.h"
#include "csocket.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/select.h>

typedef struct f2b_conn_t {
  f2b_buf_t recv;
  f2b_buf_t send;
  int sock;
  int flags;
  const char *password;
  char peer[40]; /* remote address ipv4/ipv6 */
  char challenge[40]; /* md5() of random string, nonce for auth */
} f2b_conn_t;

typedef struct f2b_sock_t {
  const char *spec;
  int sock;
  int flags;
} f2b_sock_t;

struct f2b_csock_t {
  f2b_sock_t *listen[CSOCKET_MAX_LISTEN];
  f2b_conn_t *clients[CSOCKET_MAX_CLIENTS];
  int nlisten;
  int nclients;
  char password[32];
} csock;

/* helpers */

static inline int
max(int a, int b) {
  return a > b ? a : b;
}

/* client connection-related functions */

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

void
f2b_conn_update_challenge(f2b_conn_t *conn) {
  char buf[64] = "";
  MD5_CTX md5;
  snprintf(buf, sizeof(buf), "%08lx+%ld", random(), time(NULL));
  MD5Init(&md5);
  MD5Update(&md5, (unsigned char *) buf, strlen(buf));
  MD5Final(&md5);
  strlcpy(conn->challenge, md5.hexdigest, sizeof(conn->challenge));
}

void
f2b_conn_send_unauth(f2b_conn_t *conn) {
  f2b_buf_append(&conn->send, "-unauthorized: ", 0);
  f2b_buf_append(&conn->send, conn->challenge, 0);
  f2b_buf_append(&conn->send, "\n", 0);
}

bool
f2b_conn_check_auth(f2b_conn_t *conn, f2b_cmd_t *cmd) {
  MD5_CTX md5;

  assert(conn != NULL);
  assert(cmd  != NULL);

  if (conn->flags & CSOCKET_CONN_AUTH_OK) {
    f2b_buf_append(&conn->send, "+already authorized\n", 0);
    return true;
  }

  if (strcmp(cmd->args[1], "plain") == 0) {
    if (strcmp(cmd->args[2], conn->password) == 0) {
      conn->flags |= CSOCKET_CONN_AUTH_OK;
      f2b_buf_append(&conn->send, "+ok\n", 0);
      return true;
    }
    f2b_log_msg(log_error, "csocket auth failure from %s: password mismatch", conn->peer);
  } else if (strcmp(cmd->args[1], "challenge") == 0) {
    MD5Init(&md5);
    MD5Update(&md5, (unsigned char *) conn->challenge, strlen(conn->challenge));
    MD5Update(&md5, (unsigned char *) conn->password, strlen(conn->password));
    MD5Final(&md5);
    if (strcmp(cmd->args[2], md5.hexdigest) == 0) {
      conn->flags |= CSOCKET_CONN_AUTH_OK;
      f2b_buf_append(&conn->send, "+ok\n", 0);
      return true;
    }
    f2b_log_msg(log_error, "csocket auth failure from %s: password mismatch", conn->peer);
  } else {
    f2b_log_msg(log_error, "csocket auth failure from %s: unknown auth method", conn->peer);
  }

  f2b_conn_update_challenge(conn);
  f2b_conn_send_unauth(conn);
  return false;
}

void
f2b_conn_events(f2b_conn_t *conn, f2b_cmd_t *cmd) {
  if (strcmp(cmd->args[2], "on") == 0 ||
      strcmp(cmd->args[2], "yes") == 0 ||
      strcmp(cmd->args[2], "true") == 0) {
    conn->flags |= CSOCKET_CONN_EVENTS;
    f2b_buf_append(&conn->send, "+events on\n", 0);
  } else {
    conn->flags &= ~CSOCKET_CONN_EVENTS;
    f2b_buf_append(&conn->send, "+events off\n", 0);
  }
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
        size_t len = strlen(line);
        if (len <= 0) {
          free(line);
          continue;
        } /* else: len > 0 */
        if (line[len - 1] == '\r') {
          /* strip CR */
          line[len - 1] = '\0'; len--;
        }
        f2b_log_msg(log_debug, "extracted line: %s", line);
        if ((cmd = f2b_cmd_create(line)) != NULL) {
          if (cmd->type == CMD_AUTH) {
            f2b_conn_check_auth(conn, cmd);
          } else if (cmd->type == CMD_LOG_EVENTS) {
            f2b_conn_events(conn, cmd);
          } else if (conn->flags & CSOCKET_CONN_AUTH_OK) {
            cb(cmd, &conn->send); /* handle command */
          } else {
            f2b_conn_send_unauth(conn);
          }
          f2b_cmd_destroy(cmd);
        } else if (conn->flags & CSOCKET_CONN_AUTH_OK) {
          f2b_buf_append(&conn->send, "-unknown command, try 'help'\n", 0);
        } else {
          f2b_conn_send_unauth(conn);
        }
        free(line);
      } /* while (f2b_buf_extract()) */
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

/* listen socket-related functions */

int
f2b_sock_create_unix(const char *path) {
  struct sockaddr_un addr;
  struct stat st;
  int sock = -1;
  /* is socket already exists? */
  if (stat(path, &st) == 0) {
    if ((st.st_mode & S_IFMT) == S_IFSOCK) {
      unlink(path);
    }
  }
  /* create socket  */
  if ((sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
    f2b_log_msg(log_error, "can't create control socket at %s", strerror(errno));
    unlink(path);
    return -1;
  }
  /* setup */
  memset(&addr, 0x0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strlcpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
  /* try bind */
  if (bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) != 0) {
    f2b_log_msg(log_error, "bind() on socket failed: %s -- %s", path, strerror(errno));
    unlink(path);
    return -1;
  }
  return sock;
}

int
f2b_sock_create_inet(const char *addr) {
  struct addrinfo hints;
  struct addrinfo *res;
  char host[128] = "";
  char *port = NULL;
  int ret, sock;
  strlcpy(host, addr, sizeof(host));
  /* detect host/port pair */
  if ((port = strstr(host, "]:")) != NULL) {
    /* ipv6 + port */
    *port = '\0'; port += 2;
  } else if (host[0] == '[') {
    /* ipv6 without port */
    port = CSOCKET_DEFAULT_PORT;
  } else if (strstr(host, "::") != NULL) {
    /* also ipv6 without port */
    port = CSOCKET_DEFAULT_PORT;
  } else if ((port = strstr(host, ":")) != 0) {
    /* ipv4 + port */
    *port = '\0'; port += 1;
  } else {
    /* ipv4 without port */
    port = CSOCKET_DEFAULT_PORT;
  }
  /* setup getaddrinfo() structs */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  /* resolve hostname */
  if ((ret = getaddrinfo(host, port, &hints, &res)) < 0) {
    f2b_log_msg(log_error, "getaddrinfo(): %s", gai_strerror(ret));
    return -1;
  }
  if (!res) {
    f2b_log_msg(log_error, "can't resolve hostname");
    return -1;
  }
  /* handle results */
  res[0].ai_socktype |= SOCK_NONBLOCK;
  sock = socket(res[0].ai_family, res[0].ai_socktype, res[0].ai_protocol);
  if (sock >= 0) {
    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
      f2b_log_msg(log_warn, "setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
    if (bind(sock, res[0].ai_addr, res[0].ai_addrlen) != 0) {
      f2b_log_msg(log_error, "bind() on socket failed: %s -- %s", addr, strerror(errno));
      sock = -1;
    }
  } else {
    f2b_log_msg(log_error, "can't create socket: %s", strerror(errno));
  }
  freeaddrinfo(res);
  return sock;
}

f2b_sock_t *
f2b_sock_create(const char *spec) {
  f2b_sock_t *s = NULL;
  int sock = -1;
  int flags = 0;

  assert(spec != NULL);

  if (strncmp(spec, "unix:", 5) == 0) {
    sock = f2b_sock_create_unix(spec + 5);
    flags |= CSOCKET_CONN_TYPE_UNIX;
  } else if (strncmp(spec, "inet:", 5) == 0) {
    sock = f2b_sock_create_inet(spec + 5);
    flags |= CSOCKET_CONN_TYPE_INET;
  } else {
    f2b_log_msg(log_error, "unknown type of 'listen' in config: %s", spec);
    return NULL;
  }

  if (sock < 0) {
    /* errors already logged */
    return NULL;
  }

  if (listen(sock, 5) < 0) {
    f2b_log_msg(log_error, "listen() on socket failed: %s", strerror(errno));
    return NULL;
  }

  if ((s = calloc(1, sizeof(f2b_sock_t))) != NULL) {
    f2b_log_msg(log_debug, "created control socket: %s", spec + 5);
    s->sock = sock;
    s->spec = spec;
    s->flags = flags;
    return s;
  }

  shutdown(sock, SHUT_RDWR);
  return NULL;
}

void
f2b_sock_destroy(f2b_sock_t *sock) {
  assert(sock != NULL);
  shutdown(sock->sock, SHUT_RDWR);
  if (strncmp(sock->spec, "unix:", 5) == 0)
    unlink(sock->spec + 5);
  free(sock);
}

/* control socket-related functions */

bool
f2b_csocket_create(f2b_config_section_t *config) {
  f2b_sock_t *sock = NULL;
  bool need_pass = false;

  memset(&csock, 0x0, sizeof(csock));

  for (f2b_config_param_t *p = config->param; p != NULL; p = p->next) {
    if (strcmp(p->name, "listen") == 0) {
      if (csock.nlisten >= CSOCKET_MAX_LISTEN) {
        f2b_log_msg(log_error, "ignoring excess 'listen' directive: %s", p->value);
        continue;
      }
      if ((sock = f2b_sock_create(p->value)) != NULL) {
        csock.listen[csock.nlisten] = sock;
        csock.nlisten++;
        if (strncmp(p->value, "inet:", 5) == 0)
          need_pass = true;
      }
    }
    if (strcmp(p->name, "password") == 0) {
      strlcpy(csock.password, p->value, sizeof(csock.password));
    }
  }
  if (csock.nlisten == 0) {
    f2b_csocket_destroy();
    return false;
  }
  if (need_pass && strcmp(csock.password, "") == 0) {
    snprintf(csock.password, sizeof(csock.password), "%lx+%ld", random(), time(NULL));
    f2b_log_msg(log_info, "set random password for control socket: %s", csock.password);
  }
  return true;
}

void
f2b_csocket_destroy() {
  f2b_sock_t *sock = NULL;
  f2b_conn_t *conn = NULL;

  for (int i = 0; i < CSOCKET_MAX_LISTEN; i++) {
    if ((sock = csock.listen[i]) == NULL)
      continue;
    f2b_sock_destroy(csock.listen[i]);
    csock.listen[i] = NULL;
  }
  for (int i = 0; i < CSOCKET_MAX_CLIENTS; i++) {
    if ((conn = csock.clients[i]) == NULL)
      continue;
    f2b_conn_destroy(conn);
    csock.clients[i] = NULL;
  }
  memset(&csock, 0x0, sizeof(csock));
  return;
}

void
f2b_csocket_poll(void (*cb)(const f2b_cmd_t *cmd, f2b_buf_t *res)) {
  struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
  fd_set rfds, wfds;
  f2b_conn_t *conn = NULL;
  int retval, sock, nfds = 0;
  struct sockaddr_storage addr;
  socklen_t addrlen;

  assert(cb != NULL);

  /* loop / init */
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);

  for (int i = 0; i < csock.nlisten; i++) {
    sock = csock.listen[i]->sock;
    FD_SET(sock, &rfds); /* watch for new connections */
    nfds = max(nfds, sock);
  }

  /* watch for new data on established connections */
  for (int cnum = 0; cnum < CSOCKET_MAX_CLIENTS; cnum++) {
    if ((conn = csock.clients[cnum]) == NULL)
      continue;
    FD_SET(conn->sock, &rfds);
    if (conn->send.used)
      FD_SET(conn->sock, &wfds);
    nfds = max(nfds, conn->sock);
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
  for (int i = 0; i < csock.nlisten; i++) {
    if (!FD_ISSET(csock.listen[i]->sock, &rfds))
      continue;
    /* find free connection slot */
    int cnum = 0;
    for (int cnum = 0; cnum < CSOCKET_MAX_CLIENTS; cnum++) {
      if (csock.clients[cnum] == NULL) break;
    }
    int sock = -1;
    /* accept() new connection */
    addrlen = sizeof(struct sockaddr_storage);
    if ((sock = accept(csock.listen[i]->sock, (struct sockaddr *) &addr, &addrlen)) < 0) {
      f2b_log_msg(log_error, "can't accept() new connection: %s", strerror(errno));
    } else if (cnum < CSOCKET_MAX_CLIENTS) {
      if ((conn = f2b_conn_create(RBUF_SIZE, WBUF_SIZE)) != NULL) {
        conn->flags = csock.listen[i]->flags;
        if (csock.listen[i]->flags & CSOCKET_CONN_TYPE_UNIX) {
          struct ucred peer;
          socklen_t peerlen = 0;
          peerlen = sizeof(peer);
          if (getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &peer, &peerlen) < 0) {
            f2b_log_msg(log_error, "can't get remote peer credentials: %s", strerror(errno));
          } else {
            f2b_log_msg(log_debug, "new local connection from uid %d, socket %d", peer.uid, sock);
          }
          conn->flags |= CSOCKET_CONN_AUTH_OK;
        } else /* CSOCKET_CONN_TYPE_INET */ {
          if (addr.ss_family == AF_INET) {
            inet_ntop(AF_INET,  &(((struct sockaddr_in *) &addr)->sin_addr),  conn->peer, sizeof(conn->peer));
          } else if (addr.ss_family == AF_INET6) {
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) &addr)->sin6_addr), conn->peer, sizeof(conn->peer));
          }
          f2b_log_msg(log_debug, "new remote connection from %s, socket %d", conn->peer, sock);
        }
        conn->sock = sock;
        conn->password = csock.password;
        f2b_conn_update_challenge(conn);
        csock.clients[cnum] = conn;
      } else {
        f2b_log_msg(log_error, "can't create new connection");
      }
    } else {
      f2b_log_msg(log_error, "max number of clients reached, drop connection on socket %d", sock);
      shutdown(sock, SHUT_RDWR);
    }
  }

  for (int cnum = 0; cnum < CSOCKET_MAX_CLIENTS; cnum++) {
    if ((conn = csock.clients[cnum]) == NULL)
      continue;
    retval = f2b_conn_process(conn, FD_ISSET(conn->sock, &rfds), cb);
    if (retval < 0) {
      f2b_log_msg(log_debug, "closing connection on socket %d", conn->sock);
      shutdown(conn->sock, SHUT_RDWR);
      f2b_conn_destroy(conn);
      csock.clients[cnum] = NULL;
    }
  } /* foreach connection(s) */
  return;
}

void
f2b_csocket_event_broadcast(const char *evt) {
  f2b_conn_t *conn = NULL;
  for (int cnum = 0; cnum < CSOCKET_MAX_CLIENTS; cnum++) {
    if ((conn = csock.clients[cnum]) == NULL)
      continue;
    if (conn->flags & CSOCKET_CONN_EVENTS) {
      f2b_buf_append(&conn->send, "!",  1);
      f2b_buf_append(&conn->send, evt,  0);
      f2b_buf_append(&conn->send, "\n", 1);
    }
  } /* for */
}
