/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_APPCONFIG_H_
#define F2B_APPCONFIG_H_

typedef struct f2b_appconfig_t {
  bool daemon;
  bool coredumps;
  int nice;
  uid_t uid;
  gid_t gid;
  char logdest[CONFIG_KEY_MAX];
  char config_path[PATH_MAX];
  char logfile_path[PATH_MAX];
  char csocket_path[PATH_MAX];
  char pidfile_path[PATH_MAX];
  char statedir_path[PATH_MAX];
} f2b_appconfig_t;

extern f2b_appconfig_t appconfig;

void f2b_appconfig_update(f2b_config_section_t *section);

#endif /* F2B_APPCONFIG_H_ */
