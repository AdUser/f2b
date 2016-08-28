/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "filter.h"

#define REGEX_LINE_MAX 256
#define HOST_TOKEN "<HOST>"
#define FILTER_LIBRARY_PARAM "load"

static bool
f2b_filter_load_file(f2b_filter_t *filter, const char *path) {
  FILE *f = NULL;
  size_t linenum = 0;
  char line[REGEX_LINE_MAX] = "";
  char *p, *q;

  if ((f = fopen(path, "r")) == NULL) {
    f2b_log_msg(log_error, "can't open regex list '%s': %s", path, strerror(errno));
    return false;
  }

  while (1) {
    p = fgets(line, sizeof(line), f);
    if (!p && (feof(f) || ferror(f)))
      break;
    linenum++;
    /* strip leading spaces */
    while (isblank(*p))
      p++;
    /* strip trailing spaces */
    if ((q = strchr(p, '\r')) || (q = strchr(p, '\n'))) {
      while(q > p && isspace(*q)) {
        *q = '\0';
        q--;
      }
    }
    switch(*p) {
      case '\r':
      case '\n':
      case '\0':
        /* empty line */
        break;
      case ';':
      case '#':
        /* comment line */
        break;
      default:
        if (strstr(p, HOST_TOKEN) == NULL) {
          f2b_log_msg(log_warn, "pattern at %s:%zu don't have '%s' marker, ignored", path, linenum, HOST_TOKEN);
          continue;
        }
        if (!filter->append(filter->cfg, p)) {
          f2b_log_msg(log_warn, "can't create regex from pattern at %s:%zu: %s", path, linenum, p);
          continue;
        }
        break;
    }
  }
  fclose(f);

  return true;
}

f2b_filter_t *
f2b_filter_create(f2b_config_section_t *config, const char *file) {
  f2b_config_param_t *param = NULL;
  f2b_filter_t *filter = NULL;
  int flags = RTLD_NOW | RTLD_LOCAL;
  const char *dlerr = NULL;

  assert(config != NULL);
  assert(config->type == t_filter);

  param = f2b_config_param_find(config->param, FILTER_LIBRARY_PARAM);
  if (!param) {
    f2b_log_msg(log_error, "can't find '%s' param in filter config", FILTER_LIBRARY_PARAM);
    return NULL;
  }

  if ((filter = calloc(1, sizeof(f2b_filter_t))) == NULL)
    return NULL;

  if ((filter->h = dlopen(param->value, flags)) == NULL)
     goto cleanup;
  if ((*(void **) (&filter->create)  = dlsym(filter->h, "create"))  == NULL)
    goto cleanup;
  if ((*(void **) (&filter->config)  = dlsym(filter->h, "config"))  == NULL)
    goto cleanup;
  if ((*(void **) (&filter->append)  = dlsym(filter->h, "append"))  == NULL)
    goto cleanup;
  if ((*(void **) (&filter->error)   = dlsym(filter->h, "error"))   == NULL)
    goto cleanup;
  if ((*(void **) (&filter->ready)   = dlsym(filter->h, "ready"))   == NULL)
    goto cleanup;
  if ((*(void **) (&filter->stats)   = dlsym(filter->h, "stats"))   == NULL)
    goto cleanup;
  if ((*(void **) (&filter->match)   = dlsym(filter->h, "match"))   == NULL)
    goto cleanup;
  if ((*(void **) (&filter->destroy) = dlsym(filter->h, "destroy")) == NULL)
    goto cleanup;

  /* TODO: do we need id? */
  if ((filter->cfg = filter->create("")) == NULL) {
    f2b_log_msg(log_error, "filter create config failed");
    goto cleanup;
  }

  /* try init */
  for (param = config->param; param != NULL; param = param->next) {
    if (strcmp(param->name, FILTER_LIBRARY_PARAM) == 0)
      continue;
    if (filter->config(filter->cfg, param->name, param->value))
      continue;
    f2b_log_msg(log_warn, "param pair not accepted by filter '%s': %s=%s",
      config->name, param->name, param->value);
  }

  if (!f2b_filter_load_file(filter, file))
    goto cleanup;

  if (filter->ready(filter->cfg))
    return filter;

  /* still not ready */
  f2b_log_msg(log_error, "filter '%s' not fully configured", config->name);

  cleanup:
  dlerr = dlerror();
  if (dlerr)
    f2b_log_msg(log_error, "filter load error: %s", dlerr);
  if (filter->h) {
    if (filter->cfg && filter->destroy)
      filter->destroy(filter->cfg);
    dlclose(filter->h);
  }
  free(filter);
  return NULL;
}

void
f2b_filter_destroy(f2b_filter_t *filter) {
  assert(filter != NULL);
  filter->destroy(filter->cfg);
  dlclose(filter->h);
  free(filter);
}

bool
f2b_filter_append(f2b_filter_t *filter, const char *pattern) {
  assert(filter  != NULL);
  assert(pattern != NULL);

  return filter->append(filter->cfg, pattern);
}

bool
f2b_filter_match(f2b_filter_t *filter, const char *line, char *buf, size_t buf_size) {
  assert(filter != NULL);
  assert(line   != NULL);
  assert(buf    != NULL);

  return filter->match(filter->cfg, line, buf, buf_size);
}

const char *
f2b_filter_error(f2b_filter_t *filter) {
  assert(filter != NULL);
  return filter->error(filter->cfg);
}

void
f2b_filter_stats(f2b_filter_t *filter, char *res, size_t ressize) {
  assert(filter != NULL);
  assert(res    != NULL);
  bool reset = true;
  char *pattern;
  int matches;
  char buf[256];
  const char *fmt =
    "- pattern: %s\n"
    "  matches: %d\n";
  while (filter->stats(filter->cfg, &matches, &pattern, reset)) {
    snprintf(buf, sizeof(buf), fmt, pattern, matches);
    strlcat(res, buf, ressize);
    reset = false;
  }
}
