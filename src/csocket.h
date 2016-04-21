/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_CSOCKET_H_
#define F2B_CSOCKET_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

int  f2b_csocket_create (const char *path);
void f2b_csocket_destroy(int csock, const char *path);

int  f2b_csocket_connect(const char *spath, const char *cpath);
void f2b_csocket_disconnect(int sock, const char *cpath);

int f2b_csocket_poll(int csock, void (*cb)(const f2b_cmsg_t *cmsg, char *res, size_t ressize));
int f2b_csocket_send(int csock, f2b_cmsg_t *cmsg, struct sockaddr_storage *addr, socklen_t *socklen);
int f2b_csocket_recv(int csock, f2b_cmsg_t *cmsg, struct sockaddr_storage *addr, socklen_t *socklen);

#endif /* F2B_CSOCKET_H_ */
