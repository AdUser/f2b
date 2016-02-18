#ifndef F2B_JAIL_H_
#define F2B_JAIL_H_

#include "logfile.h"
#include "match.h"

#define LOGLINE_MAX 2048

typedef struct f2b_jail_t {
  char name[32];
  f2b_match_t   *matches;
  f2b_logfile_t *logfiles;
};

#endif /* F2B_JAIL_H_ */
