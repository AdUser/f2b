/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "log.h"
#include "config.h"
#include "appconfig.h"

f2b_appconfig_t appconfig = {
  .daemon = false,
  .uid = 0,
  .gid = 0,
  .logdest = "file",
  .config_path   = "/etc/f2b/f2b.conf",
  .logfile_path  = "/var/log/f2b.log",
  .csocket_path  = DEFAULT_CSOCKET_PATH,
  .pidfile_path  = DEFAULT_PIDFILE_PATH,
  .statedir_path = DEFAULT_STATEDIR_PATH,
};

void
f2b_appconfig_update(f2b_config_section_t *section) {
  f2b_config_param_t *pa, *pb;

  if (!section)
    return;

  if ((pa = f2b_config_param_find(section->param, "user")) != NULL) {
    struct passwd *pw;
    if ((pw = getpwnam(pa->value)) != NULL)
      appconfig.uid = pw->pw_uid, appconfig.gid = pw->pw_gid;
  }
  if ((pa = f2b_config_param_find(section->param, "group")) != NULL) {
    struct group *grp;
    if ((grp = getgrnam(pa->value)) != NULL)
      appconfig.gid = grp->gr_gid;
  }

  if ((pa = f2b_config_param_find(section->param, "daemon")) != NULL) {
    appconfig.daemon = (strcmp(pa->value, "yes") == 0) ? true : false;
  }

  if ((pa = f2b_config_param_find(section->param, "pidfile")) != NULL)
    strlcpy(appconfig.pidfile_path, pa->value, sizeof(appconfig.pidfile_path));

  if ((pa = f2b_config_param_find(section->param, "csocket")) != NULL)
    strlcpy(appconfig.csocket_path, pa->value, sizeof(appconfig.csocket_path));

  if ((pa = f2b_config_param_find(section->param, "statedir")) != NULL)
    strlcpy(appconfig.statedir_path, pa->value, sizeof(appconfig.statedir_path));

  /* setup logging */
  if ((pa = f2b_config_param_find(section->param, "loglevel")) != NULL)
    f2b_log_set_level(pa->value);

  pa = f2b_config_param_find(section->param, "logdest");
  pb = f2b_config_param_find(section->param, "logfile");
  if (pa) {
    strlcpy(appconfig.logdest, pa->value, sizeof(appconfig.logdest));
    if (!appconfig.daemon && strcmp(pa->value, "stderr") == 0) {
      f2b_log_to_stderr();
    } else if (strcmp(pa->value, "file") == 0) {
      if (pb && *pb->value != '\0') {
        strlcpy(appconfig.logfile_path, pb->value, sizeof(appconfig.logfile_path));
        f2b_log_to_file(appconfig.logfile_path);
      } else {
        f2b_log_msg(log_warn, "you must set 'logfile' option with 'logdest = file'");
        f2b_log_to_syslog();
      }
    } else {
      f2b_log_to_syslog();
    }
  }

  /* TODO: */
}
