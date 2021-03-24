/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_CSOCKET_H_
#define F2B_CSOCKET_H_

#define CSOCKET_MAX_LISTEN 3
#define CSOCKET_MAX_CLIENTS 10
#define CSOCKET_DEFAULT_PORT "3370"

/* connection flags */
#define CSOCKET_CONN_TYPE_UNIX 0x01
#define CSOCKET_CONN_TYPE_INET 0x02
#define CSOCKET_CONN_NEED_AUTH 0x04
#define CSOCKET_CONN_AUTH_OK   0x08

typedef struct f2b_csock_t f2b_csock_t;

/**
 * @file
 * This file contains control socket manage routines
 */

/**
 * @brief Create control socket
 * @param spec String with socket path/address specification
 * @returns Allocated socket struct
 */
f2b_csock_t * f2b_csocket_create (f2b_config_section_t *config);

/**
 * @brief Destroy socket struct and free resources
 * @param csock Socket struct
 */
void f2b_csocket_destroy(f2b_csock_t *csock);

/**
 * @brief Poll control socket for new messages
 * @param csock Control socket fd
 * @param cb Callback for handling message
 * @returns -1 on error, 0 on no messages, and > 0 on some messages processed
 */
void f2b_csocket_poll(f2b_csock_t *csock, void (*cb)(const f2b_cmd_t *cmd, f2b_buf_t *res));

#endif /* F2B_CSOCKET_H_ */
