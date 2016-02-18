#include "jail.h"

/*
size_t
f2b_jail_poll(const jail_t *jail) {
  size_t processed = 0;
  char logline[LOGLINE_MAX] = { '\0' };

  for (f2b_logfile_t *file = jail->logfiles; file != NULL; file = file->next) {
    if (f2b_logfile_getline(file, logline) < 0)
      continue;
  }

  return processed;
}
*/
