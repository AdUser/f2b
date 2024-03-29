/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "config.h"
#include "log.h"
#include "mod-defs.h"
#include "backend.h"

#include <dlfcn.h>

#define BACKEND_LIBRARY_PARAM "load"

f2b_backend_t *
f2b_backend_create(const char *name, const char *init) {
  f2b_backend_t *backend = NULL;

  if ((backend = calloc(1, sizeof(f2b_backend_t))) == NULL)
    return NULL;

  strlcpy(backend->name, name, sizeof(backend->name));
  strlcpy(backend->init, init, sizeof(backend->init));

  return backend;
}

bool
f2b_backend_init(f2b_backend_t *backend, f2b_config_section_t *config) {
  f2b_config_param_t *param = NULL;
  int flags = RTLD_NOW | RTLD_LOCAL;
  const char *dlerr = NULL;

  assert(config != NULL);
  assert(config->type == t_backend);

  param = f2b_config_param_find(config->param, BACKEND_LIBRARY_PARAM);
  if (!param) {
    f2b_log_msg(log_error, "can't find '%s' param in backend config", BACKEND_LIBRARY_PARAM);
    return false;
  }

  if ((backend->h = dlopen(param->value, flags)) == NULL)
     goto cleanup;
  if ((*(void **) (&backend->create)  = dlsym(backend->h, "create"))  == NULL)
    goto cleanup;
  if ((*(void **) (&backend->config)  = dlsym(backend->h, "config"))  == NULL)
    goto cleanup;
  if ((*(void **) (&backend->state)   = dlsym(backend->h, "state"))   == NULL)
    goto cleanup;
  if ((*(void **) (&backend->logcb)   = dlsym(backend->h, "logcb"))   == NULL)
    goto cleanup;
  if ((*(void **) (&backend->start)   = dlsym(backend->h, "start"))   == NULL)
    goto cleanup;
  if ((*(void **) (&backend->stop)    = dlsym(backend->h, "stop"))    == NULL)
    goto cleanup;
  if ((*(void **) (&backend->ping)    = dlsym(backend->h, "ping"))    == NULL)
    goto cleanup;
  if ((*(void **) (&backend->ban)     = dlsym(backend->h, "ban"))     == NULL)
    goto cleanup;
  if ((*(void **) (&backend->check)   = dlsym(backend->h, "check"))   == NULL)
    goto cleanup;
  if ((*(void **) (&backend->unban)   = dlsym(backend->h, "unban"))   == NULL)
    goto cleanup;
  if ((*(void **) (&backend->destroy) = dlsym(backend->h, "destroy")) == NULL)
    goto cleanup;

  if ((backend->cfg = backend->create(backend->init)) == NULL) {
    f2b_log_msg(log_error, "backend create config failed");
    goto cleanup;
  }

  if ((backend->state(backend->cfg) & MOD_TYPE_BACKEND) == 0) {
    f2b_log_msg(log_error, "loaded module is not backend type");
    goto cleanup;
  }

  backend->logcb(backend->cfg, f2b_log_mod_cb);

  /* try init */
  for (param = config->param; param != NULL; param = param->next) {
    if (strcmp(param->name, BACKEND_LIBRARY_PARAM) == 0)
      continue;
    if (backend->config(backend->cfg, param->name, param->value))
      continue;
    f2b_log_msg(log_warn, "param pair not accepted by backend '%s': %s=%s",
      config->name, param->name, param->value);
  }

  if ((backend->flags = backend->state(backend->cfg)) < 0) {
    f2b_log_msg(log_error, "can't get module state");
    goto cleanup;
  }

  if (backend->flags & MOD_WRONG_API) {
    f2b_log_msg(log_error, "module reports wrong api version");
    goto cleanup;
  }

  if (backend->flags & MOD_IS_READY)
    return true;

  /* still not ready */
  f2b_log_msg(log_error, "backend '%s' not fully configured", config->name);

  cleanup:
  dlerr = dlerror();
  if (dlerr)
    f2b_log_msg(log_error, "backend load error: %s", dlerr);
  if (backend->h) {
    if (backend->cfg && backend->destroy) {
      backend->destroy(backend->cfg);
      backend->cfg = NULL;
    }
    dlclose(backend->h);
    backend->create  = NULL;
    backend->config  = NULL;
    backend->state   = NULL;
    backend->logcb   = NULL;
    backend->start   = NULL;
    backend->stop    = NULL;
    backend->ping    = NULL;
    backend->ban     = NULL;
    backend->unban   = NULL;
    backend->check   = NULL;
    backend->destroy = NULL;
    backend->h       = NULL;
  }
  return false;
}

void
f2b_backend_destroy(f2b_backend_t *backend) {
  if (!backend) return;
  if (backend->h) {
    if (backend->cfg)
      backend->destroy(backend->cfg);
    dlclose(backend->h);
    backend->h = NULL;
  }
  free(backend);
}

#define BACKEND_CMD_ARG0(CMD, RETURNS) \
RETURNS \
f2b_backend_ ## CMD(f2b_backend_t *backend) { \
  assert(backend != NULL); \
  return backend->CMD(backend->cfg); \
}

#define BACKEND_CMD_ARG1(CMD, RETURNS) \
RETURNS \
f2b_backend_ ## CMD(f2b_backend_t *backend, const char *ip) { \
  assert(backend != NULL); \
  return backend->CMD(backend->cfg, ip); \
}

BACKEND_CMD_ARG0(start, bool)
BACKEND_CMD_ARG0(stop,  bool)
BACKEND_CMD_ARG0(ping,  bool)

BACKEND_CMD_ARG1(check, bool)
BACKEND_CMD_ARG1(ban,   bool)
BACKEND_CMD_ARG1(unban, bool)
