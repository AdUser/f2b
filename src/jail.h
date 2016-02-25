#ifndef F2B_JAIL_H_
#define F2B_JAIL_H_

#include "logfile.h"
#include "ipaddr.h"

#define LOGLINE_MAX 2048

typedef struct f2b_jail_t {
  char name[32];
  time_t bantime;
  size_t tries;
  char glob[PATH_MAX];
  f2b_logfile_t *logfiles;
  f2b_ipaddr_t  *pending;
  f2b_ipaddr_t  *banned;
};

#endif /* F2B_JAIL_H_ */
