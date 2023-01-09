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

#include <libipset/session.h>
#include <libipset/types.h>
#include <libipset/ui.h>

#include "../strlcpy.h"

#include "backend.h"
#define MODNAME "ipset"

struct _config {
  char name[ID_MAX + 1];
  void (*logcb)(enum loglevel lvl, const char *msg);
  struct ipset_session *sess;
  int flags;
  bool shared;
};

#include "backend.c"

inline static bool
my_ipset_error(cfg_t *cfg, const char *func) {
  struct ipset_data *data = NULL;
  if (ipset_session_error(cfg->sess)) {
    log_msg(cfg, error, "%s: %s", func, ipset_session_error(cfg->sess));
  } else /* IPSET_WARNING */ {
    log_msg(cfg, warn, "%s: %s", func, ipset_session_warning(cfg->sess));
  }

  ipset_session_report_reset(cfg->sess);

  if ((data = ipset_session_data(cfg->sess)) != NULL)
    ipset_data_reset(data);

  return false;
}

static bool
my_ipset_cmd(cfg_t *cfg, enum ipset_cmd cmd, const char *ip) {
  const struct ipset_type *type = NULL;

  if (ipset_parse_setname(cfg->sess, IPSET_SETNAME, cfg->name) < 0)
    return my_ipset_error(cfg, "ipset_parse_setname())");

  if ((type = ipset_type_get(cfg->sess, cmd)) == NULL)
    return my_ipset_error(cfg, "ipset_type_get()");

  if (ipset_parse_elem(cfg->sess, type->last_elem_optional, ip) < 0)
    return my_ipset_error(cfg, "ipset_parse_elem()");

  if (ipset_cmd(cfg->sess, cmd, 0) < 0) {
    if (cmd == IPSET_CMD_TEST && ipset_session_error(cfg->sess) == NULL)
      return false; /* "ip is NOT in list" */
    return my_ipset_error(cfg, "ipset_cmd()");
  }

  return true;
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

  if ((cfg->sess = ipset_session_init(NULL)) == NULL) {
    log_msg(cfg, error, "can't init ipset session");
    return false;
  }

  if (ipset_session_output(cfg->sess, IPSET_LIST_NONE) < 0)
    return my_ipset_error(cfg, "ipset_session_output()");

  if (ipset_envopt_parse(cfg->sess, IPSET_ENV_EXIST, NULL) < 0)
    return my_ipset_error(cfg, "ipset_envopt_parse()");

  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->shared && usage_dec(cfg->name) > 0)
    return true;

  if (cfg->sess) {
    ipset_session_fini(cfg->sess);
    cfg->sess = NULL;
  }

  return true;
}

bool
ban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  return my_ipset_cmd(cfg, IPSET_CMD_ADD, ip);
}

bool
unban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  return my_ipset_cmd(cfg, IPSET_CMD_DEL, ip);
}

bool
check(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  return my_ipset_cmd(cfg, IPSET_CMD_TEST, ip);
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
