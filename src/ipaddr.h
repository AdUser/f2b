/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_IPADDR_H_
#define F2B_IPADDR_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * @file
 * This file contains definition of ipaddr struct and related routines
 */

#include "matches.h"

/**
 * @def IPADDR_MAX
 * Maximum text length of address
 */
#define IPADDR_MAX 48 /* 8 x "ffff" + 7 x "::" + '\0' */

/** Describes ip-address and it's metadata */
typedef struct f2b_ipaddr_t {
  struct f2b_ipaddr_t *next; /**< pointer to next addr */
  int type;                  /**< address type, AF_INET/AF_INET6 */
  char text[IPADDR_MAX];     /**< textual address */
  bool banned;               /**< is address banned, flag */
  size_t bancount;           /**< how many times this address was banned */
  time_t lastseen;           /**< self-descriptive, unixtime */
  time_t banned_at;          /**< self-descriptive, unixtime */
  time_t release_at;         /**< self-descriptive, unixtime */
  union {
    struct in_addr  v4;      /**< AF_INET address  */
    struct in6_addr v6;      /**< AF_INET6 address */
  } binary;                  /**< binary address representation, see @a type */
  f2b_matches_t matches;     /**< list of matches */
} f2b_ipaddr_t;

/**
 * @brief Create address record and fill related metadata
 * @param addr  Textual address
 * @returns Pointer to address or NULL no error
 */
f2b_ipaddr_t * f2b_ipaddr_create (const char *addr);

/**
 * @brief Free address metadata
 * @param ipaddr Pointer to f2b_ipaddr_t struct
 * @note @a ipaddr pointer becomes invalid after calling this function
 */
void f2b_ipaddr_destroy(f2b_ipaddr_t *ipaddr);
/**
 * @brief Get some stats about given address
 * @param ipaddr Pointer to f2b_ipaddr_t struct
 * @param res Buffer for storing stats
 * @param ressize Size of buffer above
 */
void f2b_ipaddr_status (f2b_ipaddr_t *ipaddr, char *res, size_t ressize);

/**
 * @brief Append address to list
 * @param list Pointer to ipaddr list (can be NULL)
 * @param ipaddr Pointer to ipaddr struct for adding to list
 * @returns Pointer to new address list
 */
f2b_ipaddr_t * f2b_addrlist_append(f2b_ipaddr_t *list, f2b_ipaddr_t *ipaddr);
/**
 * @brief Search for given ipaddr in list
 * @param list Pointer to ipaddr list (can be NULL)
 * @param addr IP address for search
 * @returns Pointer to found struct or NULL if not found
 */
f2b_ipaddr_t * f2b_addrlist_lookup(f2b_ipaddr_t *list, const char *addr);
/**
 * @brief Remove given address from list
 * @param list Pointer to ipaddr list (can be NULL)
 * @param addr IP address for remove
 * @returns Pointer to new address list or NULL if new list is empty
 */
f2b_ipaddr_t * f2b_addrlist_remove(f2b_ipaddr_t *list, const char *addr);
/**
 * @brief Free all addresses in list
 * @param list Pointer to ipaddr list (can be NULL)
 * @returns NULL
 * @note Return value not void to match other functions
 */
f2b_ipaddr_t * f2b_addrlist_destroy(f2b_ipaddr_t *list);

#endif /* F2B_IPADDR_H_ */
