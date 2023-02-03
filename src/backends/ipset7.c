/* Copyright 2017 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libipset/ipset.h>

#include "../strlcpy.h"

#include "backend.h"
#define MODNAME "ipset"

#define IP_LEN_MAX 46 /* ipv6 in mind */

struct _config {
  char name[ID_MAX + 1];
  void (*logcb)(enum loglevel lvl, const char *msg);
  struct ipset *ipset;
  enum ipset_cmd last_cmd;
  int flags;
  bool shared;
};

#include "backend.c"

/**
 * @brief Wrapper to get last ipset error and send to log
 * @note  Called from this module */
inline static bool
my_ipset_error(cfg_t *cfg, const char *func) {
  struct ipset_data *data = NULL;
  struct ipset_session *sess = ipset_session(cfg->ipset);

  if (ipset_session_report_type(sess) != IPSET_NO_ERROR) {
    log_msg(cfg, error, "%s: %s", func, ipset_session_report_msg(sess));
    ipset_session_report_reset(sess);
  }

  if ((data = ipset_session_data(sess)) != NULL)
    ipset_data_reset(data);

  return false;
}

/**
 * @note Called from library internals
 */
int
my_ipset_std_error_cb(struct ipset *ipset, void *cfg) {
  struct ipset_session *sess = ipset_session(ipset);

  switch (ipset_session_report_type(sess)) {
    case IPSET_NOTICE :
    case IPSET_WARNING :
      if (!ipset_envopt_test(sess, IPSET_ENV_QUIET))
        log_msg(cfg, error, "warning: %s", ipset_session_report_msg(sess));
      break;
    case IPSET_ERROR :
    default:
      if (((cfg_t *) cfg)->last_cmd != IPSET_CMD_TEST) {
        log_msg(cfg, error, "error: %s", ipset_session_report_msg(sess));
      }
      break;
  }

  ipset_session_report_reset(sess);
  return -1;
}

cfg_t *
create(const char *id) {
  cfg_t *cfg = NULL;

  assert(id != NULL);

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  strlcpy(cfg->name, id, sizeof(cfg->name));
  cfg->logcb = &logcb_stub;
  cfg->flags |= MOD_IS_READY;
  cfg->flags |= MOD_TYPE_BACKEND;
  cfg->last_cmd = IPSET_CMD_NONE;
  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key != NULL);
  assert(value != NULL);

  (void)(cfg);
  (void)(key);
  (void)(value);

  return false;
}

bool
start(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->shared && usage_inc(cfg->name) > 1)
    return true;

  ipset_load_types();

  if ((cfg->ipset = ipset_init()) == NULL) {
    log_msg(cfg, error, "can't init ipset library");
    return false;
  }

  if (ipset_session_output(ipset_session(cfg->ipset), IPSET_LIST_NONE) < 0)
    return my_ipset_error(cfg, "ipset_session_output()");

  if (ipset_envopt_parse(cfg->ipset, IPSET_ENV_EXIST, NULL) < 0)
    return my_ipset_error(cfg, "ipset_envopt_parse()");
  ipset_custom_printf(cfg->ipset, NULL, &my_ipset_std_error_cb, NULL, (void *) cfg);
  ipset_envopt_set(ipset_session(cfg->ipset), IPSET_ENV_QUIET);

  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->shared && usage_dec(cfg->name) > 0)
    return true;

  if (cfg->ipset) {
    ipset_fini(cfg->ipset);
    cfg->ipset = NULL;
  }

  return true;
}

bool
ban(cfg_t *cfg, const char *ip) {
  char ipw[IP_LEN_MAX] = "";
  char *argv[] = { "ignored", "add", cfg->name, ipw, NULL };
  assert(cfg != NULL);
  strlcpy(ipw, ip, sizeof(ipw));
  cfg->last_cmd = IPSET_CMD_ADD;
  return ipset_parse_argv(cfg->ipset, 4, argv) == 0;
}

bool
unban(cfg_t *cfg, const char *ip) {
  char ipw[IP_LEN_MAX] = "";
  char *argv[] = { "ignored", "del", cfg->name, ipw, NULL };
  assert(cfg != NULL);
  strlcpy(ipw, ip, sizeof(ipw));
  cfg->last_cmd = IPSET_CMD_DEL;
  return ipset_parse_argv(cfg->ipset, 4, argv) == 0;
}

bool
check(cfg_t *cfg, const char *ip) {
  char ipw[IP_LEN_MAX] = "";
  char *argv[] = { "ignored", "test", cfg->name, ipw, NULL };
  assert(cfg != NULL);
  strlcpy(ipw, ip, sizeof(ipw));
  cfg->last_cmd = IPSET_CMD_TEST;
  return ipset_parse_argv(cfg->ipset, 4, argv) == 0;
}

bool
ping(cfg_t *cfg) {
  assert(cfg != NULL);

  (void)(cfg); /* silence warning about unused variable */

  return true;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}
