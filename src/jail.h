/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_JAIL_H_
#define F2B_JAIL_H_

#include "log.h"
#include "logfile.h"
#include "ipaddr.h"
#include "config.h"
#include "filter.h"
#include "backend.h"
#include "filelist.h"

typedef struct f2b_jail_t {
  struct f2b_jail_t *next;
  bool enabled;
  time_t bantime;
  time_t findtime;
  time_t expiretime;
  size_t maxretry;
  size_t bancount;
  size_t matchcount;
  float incr_bantime;
  float incr_findtime;
  char name[CONFIG_KEY_MAX];
  char glob[PATH_MAX];
  char backend_name[CONFIG_KEY_MAX];
  char backend_init[CONFIG_VAL_MAX];
  char filter_name[CONFIG_KEY_MAX];
  char filter_init[CONFIG_VAL_MAX];
  char source_name[CONFIG_KEY_MAX];
  char source_init[CONFIG_VAL_MAX];
  f2b_logfile_t *logfiles;
  f2b_filter_t  *filter;
  f2b_ipaddr_t  *ipaddrs;
  f2b_backend_t *backend;
} f2b_jail_t;

void f2b_jail_parse_compound_value(const char *value, char *name, char *init);

f2b_jail_t *f2b_jail_create (f2b_config_section_t *section);
f2b_jail_t *f2b_jail_find   (f2b_jail_t *list, const char *name);
void   f2b_jail_set_defaults(f2b_config_section_t *section);

bool   f2b_jail_init    (f2b_jail_t *jail, f2b_config_t *config);
size_t f2b_jail_process (f2b_jail_t *jail);
bool   f2b_jail_stop    (f2b_jail_t *jail);
#endif /* F2B_JAIL_H_ */
