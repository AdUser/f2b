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
#include "appconfig.h"
#include "statefile.h"
#include "source.h"
#include "filter.h"
#include "backend.h"

/**
 * @file
 * This header describes jail definition and related routines
 */

/* jail flags */
#define JAIL_ENABLED       0x01
#define JAIL_HAS_STATE     0x02

/** jail metadata struct */
typedef struct f2b_jail_t {
  struct f2b_jail_t *next;   /**< pointer to next jail */
  char name[CONFIG_KEY_MAX]; /**< name of the jail */
  int flags;                 /**< jail flags, see above */
  size_t maxretry;           /**< option: maximum count of matches before ban */
  /* duration of misc time periods */
  time_t findtime;           /**< option: length of time period for estimating recent host activity (in seconds) */
  time_t bantime;            /**< option: host ban time on excess activity (seconds) */
  time_t expiretime;         /**< option: forget about host after this time with no activity (seconds, for banned hosts - after it's release, for not banned - after latest match) */
  /** time period length modifiers for already banned hosts */
  float findtime_extend;     /**< findtime modifier for already banned hosts in past (float) */
  float bantime_extend;      /**< bantime modifier for already banned hosts in past (float) */
  float expiretime_extend;   /**< expiretime modifier for already banned hosts in past (float) */
  /* jail stats */
  struct {
    unsigned int hosts;      /**< number of tracked hosts */
    unsigned int bans;       /**< number of ban events */
    unsigned int matches;    /**< number of match events */
  } stats;
  f2b_source_t  *source;  /**< pointer to source */
  f2b_filter_t  *filter;  /**< pointer to filter */
  f2b_backend_t *backend; /**< pointer to backend */
  f2b_statefile_t *sfile; /**< pointer to state file description */
  f2b_ipaddr_t  *ipaddrs; /**< list of known ip addresses */
} f2b_jail_t;

/**
 * @var jails
 * Global list of Defined jails
 */
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
 * @brief Set tunable parameter of jail
 * @param jail Jail pointer
 * @param param Parameter name
 * @param value Parameter value
 * @return true if parameter set, false if not found
 */
bool   f2b_jail_set_param(f2b_jail_t *jail, const char *param, const char *value);
/**
 * @brief Setup source, filter and backend in jail
 * @param jail Jail pointer
 * @param config Pointer to f2b config
 * @return true on success, false on error
 */
bool   f2b_jail_init(f2b_jail_t *jail, f2b_config_t *config);
/**
 * @brief Load state file and restore bans
 * @param jail Jail pointer
 * @returns true on success, false on error
 */
bool   f2b_jail_start(f2b_jail_t *jail);
/**
 * @brief Jail main maintenance routine
 * Polls source for data, match against filter (if set), manage matches,
 * ban ips, that exceeded their limit, unban ips after bantime expire
 * @param jail Jail for processing
 */
void   f2b_jail_process (f2b_jail_t *jail);
/**
 * @brief Correctly shutdown given jail
 * @param jail Jail pointer
 * @note Jail structure not deallocated
 */
bool   f2b_jail_stop    (f2b_jail_t *jail);

/* handlers for csocket commands processing */

/**
 * @brief Get jail status
 * @param res Response buffer
 * @param ressize Size of buffer above
 * @param jail Jail pointer
 */
void f2b_jail_cmd_status (char *res, size_t ressize, f2b_jail_t *jail);
/**
 * @brief Get jail status
 * @param res Response buffer
 * @param ressize Size of buffer above
 * @param jail Jail pointer
 * @param param Parameter name
 * @param value Parameter value
 */
void f2b_jail_cmd_set    (char *res, size_t ressize, f2b_jail_t *jail, const char *param, const char *value);
/**
 * @brief ipaddr manage routine in given jail
 * @param res Response buffer
 * @param ressize Size of buffer above
 * @param jail Jail pointer
 * @param op Operation for ipaddr >0 - ban, 0 - check, <0 - unban
 * @param ip Ip address
 */
void f2b_jail_cmd_ip_xxx (char *res, size_t ressize, f2b_jail_t *jail, int op, const char *ip);

#endif /* F2B_JAIL_H_ */
