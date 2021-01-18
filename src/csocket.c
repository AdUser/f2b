/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "commands.h"
#include "csocket.h"
#include "log.h"

int
f2b_csocket_create(const char *path) {
  struct sockaddr_un addr;
  int csock = -1;
  int flags = -1;

  assert(path != NULL);

  if ((csock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    f2b_log_msg(log_error, "can't create control socket: %s", strerror(errno));
    return -1;
  }

  memset(&addr, 0x0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strlcpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  if ((flags = fcntl(csock, F_GETFL, 0)) < 0)
    return -1;
  if (fcntl(csock, F_SETFL, flags | O_NONBLOCK) < 0) {
    f2b_log_msg(log_error, "can't set non-blocking mode on socket: %s", strerror(errno));
    return -1;
  }

  unlink(path);
  if (bind(csock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) != 0) {
    f2b_log_msg(log_error, "bind() on socket failed: %s", strerror(errno));
    return -1;
  }

  return csock;
}

void
f2b_csocket_destroy(int sock, const char *path) {
  assert(path != NULL);

  if (sock >= 0)
    close(sock);
  unlink(path);

  return;
}

int
f2b_csocket_connect(const char *spath, const char *cpath) {
  struct sockaddr_un caddr, saddr;
  int csock = -1;

  assert(spath != NULL);
  assert(cpath != NULL);

  memset(&saddr, 0x0, sizeof(caddr));
  memset(&caddr, 0x0, sizeof(saddr));

  caddr.sun_family = AF_LOCAL;
  strlcpy(caddr.sun_path, cpath, sizeof(caddr.sun_path));
  saddr.sun_family = AF_LOCAL;
  strlcpy(saddr.sun_path, spath, sizeof(saddr.sun_path));

  if ((csock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    f2b_log_msg(log_error, "can't create control socket");
    return -1;
  }

  if (bind(csock, (struct sockaddr *) &caddr, sizeof(struct sockaddr_un)) != 0) {
    f2b_log_msg(log_error, "bind() to local side of socket failed: %s", strerror(errno));
    return -1;
  }

  if (connect(csock, (struct sockaddr *) &saddr, sizeof(struct sockaddr_un)) != 0) {
    f2b_log_msg(log_error, "connect() to socket failed: %s", strerror(errno));
    return -1;
  }

  return csock;
}

void
f2b_csocket_disconnect(int sock, const char *cpath) {
  unlink(cpath);
  if (sock >= 0)
    close(sock);
  return;
}

void
f2b_csocket_rtimeout(int sock, float timeout) {
  int ret = 0;
  struct timeval tv;
  tv.tv_sec  = (int) timeout;
  tv.tv_usec = (int) ((timeout - tv.tv_sec) * 1000000);
  ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *) &tv, sizeof(struct timeval));
  if (ret == 0)
    return;
  f2b_log_msg(log_warn, "can't set recv timeout for csocket: %s", strerror(errno));
}

const char *
f2b_csocket_error(int retcode) {
  const char *err = "no error";
  switch (retcode) {
    case -1 : err = strerror(errno); break;
    case -2 : err = "damaged cmsg on socket: truncated"; break;
    case -3 : err = "damaged cmsg on socket: no magic"; break;
    case -4 : err = "damaged cmsg on socket: version mismatch"; break;
    case -5 : err = "damaged cmsg on socket: unknown command type"; break;
    case -6 : err = "damaged cmsg on socket: size mismatch"; break;
    default : err = "unknown cmsg error"; break;
  }
  return err;
}

int
f2b_csocket_recv(int csock, f2b_cmsg_t *cmsg, struct sockaddr_storage *addr, socklen_t *addrlen) {
  struct msghdr msg;
  uint16_t size;
  int ret;

  assert(csock >= 0);
  assert(cmsg != NULL);

  struct iovec iov[] = {
    { .iov_len = sizeof(cmsg->magic),   .iov_base = &cmsg->magic[0] },
    { .iov_len = sizeof(cmsg->version), .iov_base = &cmsg->version },
    { .iov_len = sizeof(cmsg->type),    .iov_base = &cmsg->type },
    { .iov_len = sizeof(cmsg->flags),   .iov_base = &cmsg->flags },
    { .iov_len = sizeof(cmsg->size),    .iov_base = &size /* need ntohs */ },
    { .iov_len = sizeof(cmsg->pass),    .iov_base = &cmsg->pass[0] },
    { .iov_len = sizeof(cmsg->data),    .iov_base = &cmsg->data[0] },
  };

  memset(&msg, 0x0, sizeof(msg));
  msg.msg_name    = addr;
  msg.msg_namelen = *addrlen;
  msg.msg_iov     = iov;
  msg.msg_iovlen  = 7;

  ret = recvmsg(csock, &msg, 0);
  if (ret < 0 && errno == EAGAIN)
    return 0; /* non-blocking mode & no messages */
  if (ret < 0)
    return -1; /* recvmsg() error, see errno */
  if (msg.msg_flags & MSG_TRUNC)
    return -2; /* truncated */
  if (memcmp(cmsg->magic, "F2B", 3) != 0)
    return -3; /* no magic */
  if (cmsg->version != F2B_PROTO_VER)
    return -4; /* version mismatch */
  if (cmsg->type >= CMD_MAX_NUMBER)
    return -5; /* unknown command */
  cmsg->size = ntohs(size);
  if (ret != (cmsg->size + 16))
    return -6; /* size mismatch */
  *addrlen = msg.msg_namelen;

  return ret;
}

int
f2b_csocket_send(int csock, f2b_cmsg_t *cmsg, struct sockaddr_storage *addr, socklen_t *addrlen) {
  struct msghdr msg;
  uint16_t size;
  int ret;

  assert(csock >= 0);
  assert(cmsg != NULL);

  struct iovec iov[] = {
    { .iov_len = sizeof(cmsg->magic),   .iov_base = &cmsg->magic[0] },
    { .iov_len = sizeof(cmsg->version), .iov_base = &cmsg->version },
    { .iov_len = sizeof(cmsg->type),    .iov_base = &cmsg->type },
    { .iov_len = sizeof(cmsg->flags),   .iov_base = &cmsg->flags },
    { .iov_len = sizeof(cmsg->size),    .iov_base = &size /* need htons */ },
    { .iov_len = sizeof(cmsg->pass),    .iov_base = &cmsg->pass[0] },
    { .iov_len = cmsg->size,            .iov_base = &cmsg->data[0] },
  };
  size = htons(cmsg->size);

  memset(&msg, 0x0, sizeof(msg));
  msg.msg_name    = addr;
  msg.msg_namelen = *addrlen;
  msg.msg_iov     = iov;
  msg.msg_iovlen  = 7;

  if ((ret = sendmsg(csock, &msg, 0)) <= 0)
    return -1; /* see errno */

  return ret;
}

int
f2b_csocket_poll(int csock, void (*cb)(const f2b_cmd_t *cmd, f2b_buf_t *res)) {
  char res[DATA_LEN_MAX + 1];
  f2b_cmsg_t cmsg;
  struct sockaddr_storage addr;
  socklen_t addrlen;
  int ret, processed = 0;

  assert(csock >= 0);
  assert(cb != NULL);

  while (1) {
    memset(&cmsg, 0x0, sizeof(cmsg));
    memset(&addr, 0x0, sizeof(addr));
    addrlen = sizeof(addr);
    ret = f2b_csocket_recv(csock, &cmsg, &addr, &addrlen);
    if (ret == 0)
      break; /* no messages */
    if (ret < 0) {
      f2b_log_msg(log_error, "%s", f2b_csocket_error(ret));
    }
    /* TODO: check auth */
    cb(&cmsg, res, sizeof(res));
    if (cmsg.flags & CMSG_FLAG_NEED_REPLY) {
      memset(&cmsg, 0x0, sizeof(cmsg));
      strncpy(cmsg.magic, "F2B", sizeof(cmsg.magic));
      strlcpy(cmsg.data,  res,   sizeof(cmsg.data));
      cmsg.version = F2B_PROTO_VER;
      cmsg.type = CMD_RESP;
      cmsg.size = strlen(res);
      cmsg.data[cmsg.size] = '\0';
      ret = f2b_csocket_send(csock, &cmsg, &addr, &addrlen);
    }
    processed++;
  }

  return processed;
}
