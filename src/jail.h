#ifndef F2B_JAIL_H_
#define F2B_JAIL_H_

#include "log.h"
#include "logfile.h"
#include "ipaddr.h"
#include "config.h"

typedef struct f2b_jail_t {
  bool enabled;
  time_t bantime;
  size_t tries;
  char name[CONFIG_KEY_MAX];
  char backend[CONFIG_KEY_MAX];
  char backend_init[CONFIG_VAL_MAX];
  char glob[PATH_MAX];
  f2b_logfile_t *logfiles;
  f2b_ipaddr_t  *ipaddrs;
} f2b_jail_t;

#endif /* F2B_JAIL_H_ */
