/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_CSOCKET_H_
#define F2B_CSOCKET_H_

/**
 * @file
 * This file contains control socket manage routines
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

/**
 * @brief Create UNIX socket with given path
 * @param path Path to socket endpoint
 * @returns Socket fd
 */
int  f2b_csocket_create (const char *path);
/**
 * @brief Close UNIX socket and unlink endpoint
 * @param csock Socket fd
 * @param path Path to socket endpoint
 */
void f2b_csocket_destroy(int csock, const char *path);

/**
 * @brief Connect to given socket
 * @param spath path to control socket endpoint
 * @param cpath Path to client socket's endpoint
 * @returns Connected fd or -1 on error
 */
int  f2b_csocket_connect(const char *spath, const char *cpath);
/**
 * @brief Close client connection and unlink client's endpoint
 * @param csock Socket fd
 * @param cpath Path to client socket's endpoint
 */
void f2b_csocket_disconnect(int csock, const char *cpath);

/**
 * @brief Set recieve rimeout on socket
 * @param csock Socket fd
 * @param timeout Timeout in seconds
 */
void f2b_csocket_rtimeout(int csock, float timeout);

/**
 * @brief Get error description for f2b_csocket_recv()
 * @param retcode Return code fromf2b_csocket_recv()
 * @returns Pointer to errro description
 */
const char *
f2b_csocket_error(int retcode);

/**
 * @brief Poll control socket for new messages
 * @param csock Control socket fd
 * @param cb Callback for handling message
 * @returns -1 on error, 0 on no messages, and > 0 on some messages processed
 */
int f2b_csocket_poll(int csock, void (*cb)(const f2b_cmsg_t *cmsg, char *res, size_t ressize));
/**
 * @brief Pack and send control message
 * @param csock Opened socket fd
 * @param cmsg  Control message pointer
 * @param addr  Pointer for destination address store
 * @param addrlen  Size of address storage
 * @returns >0 on success
 */
int f2b_csocket_send(int csock, f2b_cmsg_t *cmsg, struct sockaddr_storage *addr, socklen_t *addrlen);
/**
 * @brief Recieve and unpack control message
 * @param csock Opened socket fd
 * @param cmsg  Control message pointer
 * @param addr  Pointer for sender address store
 * @param addrlen  Size of address storage
 * @returns >0 on success, 0 on no avalilable messages, <0 on error
 */
int f2b_csocket_recv(int csock, f2b_cmsg_t *cmsg, struct sockaddr_storage *addr, socklen_t *addrlen);

#endif /* F2B_CSOCKET_H_ */
