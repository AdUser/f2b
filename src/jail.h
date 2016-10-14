/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_JAIL_H_
#define F2B_JAIL_H_

#include "log.h"
#include "ipaddr.h"
#include "config.h"
#include "source.h"
#include "filter.h"
#include "backend.h"

typedef struct f2b_jail_t {
  struct f2b_jail_t *next;   /**< pointer to next jail */
  bool enabled;              /**< option: is jail enabled */
  time_t bantime;            /**< option: ban host for this time if maxretry exceeded */
  time_t findtime;           /**< option: time period for counting matches */
  time_t expiretime;         /**< option: forget about host after this time with on activity (not including bantime) */
  size_t maxretry;           /**< option: maximum count of matches before ban */
  size_t bancount;           /**< stats: total number of bans for this jail */
  size_t matchcount;         /**< stats: total number of matches for this jail */
  float incr_bantime;        /**< option: multiplier for bantime  */
  float incr_findtime;       /**< option: multiplier for finetime */
  char name[CONFIG_KEY_MAX]; /**< name of the jail */
  char backend_name[CONFIG_KEY_MAX]; /**< backend name from config (eg [backend:$NAME] section) */
  char backend_init[CONFIG_VAL_MAX]; /**< backend init string (eg `backend = NAME:$INIT_STRING` line from jail section) */
  char filter_name[CONFIG_KEY_MAX];  /**< filter name from config (eg [filter:$NAME] section) */
  char filter_init[CONFIG_VAL_MAX];  /**< filter init string (eg `filter = NAME:$INIT_STRING` line from jail section) */
  char source_name[CONFIG_KEY_MAX];  /**< source name from config (eg [source:$NAME] section) */
  char source_init[CONFIG_VAL_MAX];  /**< source init string (eg `source = NAME:$INIT_STRING` line from jail section) */
  f2b_source_t  *source;  /**< pointer to source */
  f2b_filter_t  *filter;  /**< pointer to filter */
  f2b_backend_t *backend; /**< pointer to backend */
  f2b_ipaddr_t  *ipaddrs; /**< list of known ip addresses */
} f2b_jail_t;

/** defined jails list */
extern f2b_jail_t *jails;

/**
 * @brief Apply defaults to jail template (affects later f2b_jail_create())
 * @param section 'defaults' section from config
 */
void f2b_jail_set_defaults(f2b_config_section_t *section);
/**
 * @brief Create jail struct and init it's metadata
 * @param section Jail config section
 * @return Pointer to allocated jail or NULL on error
 */
f2b_jail_t *f2b_jail_create (f2b_config_section_t *section);
/**
 * @brief Find jail in jail list by name
 * @param list Jails list
 * @param name Jail name
 * @returns Pointer to wanted jail or NULL if not found
 */
f2b_jail_t *f2b_jail_find (f2b_jail_t *list, const char *name);

/**
 * @brief Setup source, filter and backend in jail
 * @param jail Jail pointer
 * @param config Pointer to f2b config
 * @return true on success, false on error
 */
bool   f2b_jail_init(f2b_jail_t *jail, f2b_config_t *config);
/**
 * @brief Jail maintenance routine
 * Polls source for data, match against filter, manage matches,
 * ban ips, that exceeded their limit, unban ips after bantime expire
 * @param jail Jail for processing
 */
size_t f2b_jail_process (f2b_jail_t *jail);
/**
 * @brief Correctly shutdown given jail
 * @param jail Jail pointer
 * @note Jail structure not deallocated
 */
bool   f2b_jail_stop    (f2b_jail_t *jail);

/* handlers for cmsg */

/**
 * @brief Get jail status
 * @param res Response buffer
 * @param ressize Size of buffer above
 * @param Jail pointer
 */
void f2b_jail_cmd_status (char *res, size_t ressize, f2b_jail_t *jail);
/**
 * @brief ipaddr manage routine in given jail
 * @param res Response buffer
 * @param ressize Size of buffer above
 * @param Jail pointer
 * @param op Operation for ipaddr >0 - ban, 0 - check, <0 - unban
 * @param ip Ip address
 */
void f2b_jail_cmd_ip_xxx (char *res, size_t ressize, f2b_jail_t *jail, int op, const char *ip);

#endif /* F2B_JAIL_H_ */
