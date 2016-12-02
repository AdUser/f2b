/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_STATEFILE_H_
#define F2B_STATEFILE_H_

typedef struct f2b_statefile_t {
  char path[PATH_MAX];
  bool need_save;
} f2b_statefile_t;

f2b_statefile_t *
f2b_statefile_create(const char *statedir, const char *jailname);

void
f2b_statefile_destroy(f2b_statefile_t *sf);

f2b_ipaddr_t *
f2b_statefile_load(f2b_statefile_t *sf, size_t matches);

bool
f2b_statefile_save(f2b_statefile_t *sf, f2b_ipaddr_t *addrlist);

#endif /* F2B_STATEFILE_H_ */
