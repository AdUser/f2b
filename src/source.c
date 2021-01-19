/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "source.h"

#define SOURCE_LIBRARY_PARAM "load"

f2b_source_t *
f2b_source_create(f2b_config_section_t *config, const char *init) {
  f2b_config_param_t *param = NULL;
  f2b_source_t *source = NULL;
  int flags = RTLD_NOW | RTLD_LOCAL;
  const char *dlerr = NULL;

  assert(config != NULL);
  assert(config->type == t_source);

  param = f2b_config_param_find(config->param, SOURCE_LIBRARY_PARAM);
  if (!param) {
    f2b_log_msg(log_error, "can't find '%s' param in source config", SOURCE_LIBRARY_PARAM);
    return NULL;
  }

  if ((source = calloc(1, sizeof(f2b_source_t))) == NULL)
    return NULL;

  if ((source->h = dlopen(param->value, flags)) == NULL)
     goto cleanup;
  if ((*(void **) (&source->create)  = dlsym(source->h, "create"))  == NULL)
    goto cleanup;
  if ((*(void **) (&source->config)  = dlsym(source->h, "config"))  == NULL)
    goto cleanup;
  if ((*(void **) (&source->ready)   = dlsym(source->h, "ready"))   == NULL)
    goto cleanup;
  if ((*(void **) (&source->logcb)   = dlsym(source->h, "logcb"))   == NULL)
    goto cleanup;
  if ((*(void **) (&source->start)   = dlsym(source->h, "start"))   == NULL)
    goto cleanup;
  if ((*(void **) (&source->next)    = dlsym(source->h, "next"))    == NULL)
    goto cleanup;
  if ((*(void **) (&source->stats)   = dlsym(source->h, "stats"))   == NULL)
    goto cleanup;
  if ((*(void **) (&source->stop)    = dlsym(source->h, "stop"))    == NULL)
    goto cleanup;
  if ((*(void **) (&source->destroy) = dlsym(source->h, "destroy")) == NULL)
    goto cleanup;

  if ((source->cfg = source->create(init)) == NULL) {
    f2b_log_msg(log_error, "source create config failed");
    goto cleanup;
  }

  source->logcb(source->cfg, f2b_log_mod_cb);

  /* try init */
  for (param = config->param; param != NULL; param = param->next) {
    if (strcmp(param->name, SOURCE_LIBRARY_PARAM) == 0)
      continue;
    if (source->config(source->cfg, param->name, param->value))
      continue;
    f2b_log_msg(log_warn, "param pair not accepted by source '%s': %s=%s",
      config->name, param->name, param->value);
  }

  if (source->ready(source->cfg))
    return source;

  /* still not ready */
  f2b_log_msg(log_error, "source '%s' not fully configured", config->name);

  cleanup:
  dlerr = dlerror();
  if (dlerr)
    f2b_log_msg(log_error, "source load error: %s", dlerr);
  if (source->h) {
    if (source->cfg && source->destroy)
      source->destroy(source->cfg);
    dlclose(source->h);
  }
  free(source);
  return NULL;
}

void
f2b_source_destroy(f2b_source_t *source) {
  assert(source != NULL);
  source->destroy(source->cfg);
  dlclose(source->h);
  free(source);
}

bool
f2b_source_next(f2b_source_t *source, char *buf, size_t bufsize, bool reset) {
  assert(source != NULL);
  return source->next(source->cfg, buf, bufsize, reset);
}

#define SOURCE_CMD_ARG0(CMD, RETURNS) \
RETURNS \
f2b_source_ ## CMD(f2b_source_t *source) { \
  assert(source != NULL); \
  return source->CMD(source->cfg); \
}

SOURCE_CMD_ARG0(start, bool)
SOURCE_CMD_ARG0(stop,  bool)
SOURCE_CMD_ARG0(ready, bool)

void
f2b_source_cmd_stats(char *buf, size_t bufsize, f2b_source_t *source) {
  assert(source != NULL);
  assert(buf    != NULL);

  source->stats(source->cfg, buf, bufsize);
}
