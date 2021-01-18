/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_CSOCKET_H_
#define F2B_CSOCKET_H_

#define MAXCONNS 5

typedef struct f2b_csock_t f2b_csock_t;

/**
 * @file
 * This file contains control socket manage routines
 */

/**
 * @brief Create UNIX socket with given path
 * @param path Path to socket endpoint
 * @returns Socket fd
 */
f2b_csock_t * f2b_csocket_create (const char *path);
/**
 * @brief Close UNIX socket and unlink endpoint
 * @param csock Socket fd
 * @param path Path to socket endpoint
 */
void f2b_csocket_destroy(f2b_csock_t *csock);

/**
 * @brief Poll control socket for new messages
 * @param csock Control socket fd
 * @param cb Callback for handling message
 * @returns -1 on error, 0 on no messages, and > 0 on some messages processed
 */
int f2b_csocket_poll(f2b_csock_t *csock, void (*cb)(const f2b_cmd_t *cmd, f2b_buf_t *res));

#endif /* F2B_CSOCKET_H_ */
