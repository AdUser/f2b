#ifndef F2B_JAIL_H_
#define F2B_JAIL_H_

#include "log.h"
#include "logfile.h"
#include "ipaddr.h"
#include "config.h"
#include "regexps.h"
#include "backend.h"
#include "filelist.h"

typedef struct f2b_jail_t {
  struct f2b_jail_t *next;
  bool enabled;
  time_t bantime;
  size_t tries;
  char name[CONFIG_KEY_MAX];
  char glob[PATH_MAX];
  char backend_name[CONFIG_KEY_MAX];
  char backend_init[CONFIG_VAL_MAX];
  char filter_name[CONFIG_KEY_MAX];
  char filter_init[CONFIG_VAL_MAX];
  char source_name[CONFIG_KEY_MAX];
  char source_init[CONFIG_VAL_MAX];
  f2b_logfile_t *logfiles;
  f2b_regex_t   *regexps;
  f2b_ipaddr_t  *ipaddrs;
  f2b_backend_t *backend;
} f2b_jail_t;

f2b_jail_t *f2b_jail_create (f2b_config_section_t *section);
void   f2b_jail_set_defaults(f2b_config_section_t *section);

size_t f2b_jail_process (f2b_jail_t *jail);
bool   f2b_jail_init    (f2b_jail_t *jail, f2b_config_t *config);
#endif /* F2B_JAIL_H_ */
