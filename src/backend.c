#include "common.h"
#include "backend.h"

#define BACKEND_LIBRARY_PARAM "load"

f2b_backend_t *
f2b_backend_create(f2b_config_section_t *config) {
  f2b_config_param_t *param = NULL;
  f2b_backend_t *backend = NULL;
  int flags = RTLD_NOW | RTLD_LOCAL;

  assert(config != NULL);
  assert(config->type == t_backend);

  f2b_config_find_param(config->param, BACKEND_LIBRARY_PARAM);
  if (!param)
    return NULL;

  if ((backend = calloc(1, sizeof(f2b_backend_t))) == NULL)
    return NULL;

  if ((backend->h = dlopen(param->value, flags)) == NULL)
     goto cleanup;
  if ((*(void **) (&backend->init)   = dlsym(backend->h, "init"))   == NULL)
    goto cleanup;
  if ((*(void **) (&backend->ready)  = dlsym(backend->h, "ready"))  == NULL)
    goto cleanup;
  if ((*(void **) (&backend->ban)    = dlsym(backend->h, "ban"))    == NULL)
    goto cleanup;
  if ((*(void **) (&backend->unban)  = dlsym(backend->h, "unban"))  == NULL)
    goto cleanup;
  if ((*(void **) (&backend->exists) = dlsym(backend->h, "exists")) == NULL)
    goto cleanup;

  if (backend->ready)
    return backend;

  /* try init */
  for (; param != NULL; param = param->next) {
    if (strcmp(param->name, BACKEND_LIBRARY_PARAM) == 0)
      continue;
    if (!backend->init(param->name, param->value)) {
      f2b_log_msg(log_warn, "param pair not accepted by backend '%s': %s=%s",
        config->name, param->name, param->value);
    }
  }

  if (backend->ready)
    return backend;

  /* still not ready */
  f2b_log_msg(log_error, "backend '%s' not fully configured", config->name);

  cleanup:
  if (backend->h)
    dlclose(backend->h);
  free(backend);
  return NULL;
}

void
f2b_backend_destroy(f2b_backend_t *backend) {
  assert(backend != NULL);
  dlclose(backend->h);
  free(backend);
}
