/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "config.h"
#include "log.h"

#include <glob.h>

f2b_config_param_t *
f2b_config_param_create(const char *src) {
  f2b_config_param_t *param = NULL;
  char line[CONFIG_LINE_MAX] = "";
  char *p, *key, *value;
  size_t len;

  strncpy(line, src, sizeof(line));
  line[CONFIG_LINE_MAX - 1] = '\0';

  /* strip spaces before key */
  key = line;
  while (isblank(*key))
    key++;

  if ((value = strchr(key, '=')) == NULL)
    return NULL;

  /* strip spaces after key */
  p = value - 1;
  while (p > line && isblank(*p))
    p--;
  p++, *p = '\0';

  value++; /* move to next char after '=' */
  while (isblank(*value))
    value++;

  /* strip trailing comment */
  if ((p = strstr(value,  " #")) != NULL) *p = '\0';
  if ((p = strstr(value, "\t#")) != NULL) *p = '\0';
  if ((p = strstr(value,  " ;")) != NULL) *p = '\0';
  if ((p = strstr(value, "\t;")) != NULL) *p = '\0';

  /* strip trailing spaces */
  p = value + strlen(value);
  if (p > value)
    p--; /* step back at char before '\0' */
  while (p > value && isspace(*p))
    p--;
  p++, *p = '\0';

  len = strlen(key);
  if (len < 1 || len >= CONFIG_KEY_MAX)
    return NULL;

  len = strlen(value);
  if (len < 1 || len >= CONFIG_VAL_MAX)
    return NULL;

  if ((param = calloc(1, sizeof(f2b_config_param_t))) != NULL) {
    strncpy(param->name,  key,   sizeof(param->name));
    strncpy(param->value, value, sizeof(param->value));
    param->name [CONFIG_KEY_MAX - 1] = '\0';
    param->value[CONFIG_VAL_MAX - 1] = '\0';
    return param;
  }

  return NULL;
}

f2b_config_param_t *
f2b_config_param_find(f2b_config_param_t *param, const char *name) {
  for (; param != NULL; param = param->next) {
    if (strcmp(name, param->name) == 0)
      return param;
  }

  return NULL;
}

f2b_config_param_t *
f2b_config_param_append(f2b_config_param_t *list, f2b_config_param_t *param, bool replace) {
  f2b_config_param_t *p;

  assert(param != NULL);

  if (!list)
    return param; /* no parameters yet */

  if (replace && (p = f2b_config_param_find(list, param->name)) != NULL) {
    /* found param with same name */
    strncpy(p->value, param->value, sizeof(p->value));
    free(param);
    return list;
  }

  for (p = list; p->next != NULL; p = p->next)
    /* find last element */;

  p->next = param;
  return list;
}

f2b_config_section_t *
f2b_config_section_create(const char *src) {
  f2b_config_section_t *section = NULL;
  char line[CONFIG_LINE_MAX] = "";
  char *name, *end;

  assert(src != NULL);
  assert(*src == '[');

  src++;
  strncpy(line, src, sizeof(line));
  line[CONFIG_LINE_MAX - 1] = '\0';

  if ((end = strchr(line, ']')) == NULL)
    return NULL;
  *end = '\0';

  if ((section = calloc(1, sizeof(f2b_config_section_t))) == NULL)
    return NULL;

  name = "main";
  if (strncmp(line, name, strlen(name)) == 0) {
    section->type = t_main;
    return section;
  }

  name = "defaults";
  if (strncmp(line, name, strlen(name)) == 0) {
    section->type = t_defaults;
    return section;
  }

  name = "backend:";
  if (strncmp(line, name, strlen(name)) == 0) {
    section->type = t_backend;
    strncpy(section->name, line + strlen(name), sizeof(section->name));
    return section;
  }

  name = "filter:";
  if (strncmp(line, name, strlen(name)) == 0) {
    section->type = t_filter;
    strncpy(section->name, line + strlen(name), sizeof(section->name));
    return section;
  }

  name = "jail:";
  if (strncmp(line, name, strlen(name)) == 0) {
    section->type = t_jail;
    strncpy(section->name, line + strlen(name), sizeof(section->name));
    return section;
  }

  free(section);
  return NULL;
}

f2b_config_section_t *
f2b_config_section_find(f2b_config_section_t *section, const char *name) {
  for (; section != NULL; section = section->next) {
    if (strcmp(section->name, name) == 0)
      return section;
  }

  return NULL;
}

bool
f2b_config_load(f2b_config_t *config, const char *path, bool recursion) {
  f2b_config_section_t *section = NULL; /* always points to current section */
  f2b_config_param_t   *param   = NULL; /* temp pointer */
  FILE *f = NULL; /* config file fd */
  char *p; /* temp pointer */
  char line[CONFIG_LINE_MAX] = ""; /* last read line */
  bool skip_section = true; /* if set - skip parameters unless next section */
  size_t linenum = 0; /* current line number in config */

  assert(config != NULL);

  if ((f = fopen(path, "r")) == NULL) {
    f2b_log_msg(log_error, "can't open config file '%s': %s", path, strerror(errno));
    return false;
  }

  f2b_log_msg(log_debug, "processing config file: %s", path);

  while (1) {
    p = fgets(line, sizeof(line), f);
    if (!p && (feof(f) || ferror(f)))
      break;
    if ((p = strchr(line, '\r')) != NULL)
      *p = '\0';
    if ((p = strchr(line, '\n')) != NULL)
      *p = '\0';
    p = line;
    while (isblank(*p))
      p++;
    linenum++;
    switch(*p) {
      case '\0':
      case '\r':
      case '\n':
        /* empty line */
        break;
      case ';':
      case '#':
        /* comment line */
        break;
      case '[':
        /* section header */
        section = f2b_config_section_create(p);
        if (section) {
          skip_section = false;
          section = f2b_config_section_append(config, section);
        } else {
          skip_section = true;
          f2b_log_msg(log_error, "unknown section at line %zu: %s", linenum, p);
        }
        break;
      default:
        if (skip_section) {
          f2b_log_msg(log_warn, "skipping line in unknown section: %s", p);
          continue;
        }
        /* key/value pair */
        param = f2b_config_param_create(p);
        if (param && (section->type == t_main || section->type == t_defaults)) {
          section->param = f2b_config_param_append(section->param, param, true);
        } else if (param) {
          section->param = f2b_config_param_append(section->param, param, false);
        } else {
          f2b_log_msg(log_error, "can't parse key/value at line %zu: %s", linenum, p);
          continue;
        }
        break;
    } /* switch */
  } /* while */
  fclose(f);

  if (recursion && config->main && (param = f2b_config_param_find(config->main->param, "includes"))) {
    struct stat st;
    int ret = 0;
    if (stat(param->value, &st) != 0) {
      f2b_log_msg(log_warn, "path in 'includes' option not exists, ignored");
      return true;
    }
    if (!S_ISDIR(st.st_mode)) {
      f2b_log_msg(log_warn, "path in 'includes' option not a directory, ignored");
      return true;
    }
    /* process dir */
    char pattern[PATH_MAX] = "";
    glob_t globbuf;
    snprintf(pattern, sizeof(pattern), "%s/*.conf", param->value);
    ret = glob(pattern, 0, NULL, &globbuf);
    if (ret == GLOB_NOMATCH)
      return true;
    if (ret != 0) {
      f2b_log_msg(log_error, "glob on 'includes' dir failed");
      return false;
    }
    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
      f2b_config_load(config, globbuf.gl_pathv[i], false);
      /* TODO: includes processing are not fatal? hmm... good question */
    }
    globfree(&globbuf);
  }

  return true;
}

#define FREE_SECTIONS(SECTION) \
  for (; SECTION != NULL; SECTION = ns) { \
    ns = SECTION->next; \
    for (; SECTION->param != NULL; \
           SECTION->param = np) { \
      np = SECTION->param->next; \
      FREE(SECTION->param); \
    } \
  }

void
f2b_config_free(f2b_config_t *config) {
  f2b_config_section_t *ns = NULL; /* next section */
  f2b_config_param_t   *np = NULL; /* next param */

  FREE_SECTIONS(config->main);
  FREE_SECTIONS(config->defaults);
  FREE_SECTIONS(config->filters);
  FREE_SECTIONS(config->backends);
  FREE_SECTIONS(config->jails);
}

f2b_config_section_t *
f2b_config_section_append(f2b_config_t *config, f2b_config_section_t *section) {
  f2b_config_section_t *prev = NULL;
  f2b_config_section_t **s   = NULL;

  assert(config != NULL);
  assert(section != NULL);

  switch (section->type) {
    case t_main:     s = &config->main;     break;
    case t_defaults: s = &config->defaults; break;
    case t_filter:   s = &config->filters;  break;
    case t_backend:  s = &config->backends; break;
    case t_jail:     s = &config->jails;    break;
    default:
      f2b_log_msg(log_error, "unknown section type");
      abort();
      break;
  }

  if ((prev = f2b_config_section_find(*s, section->name)) != NULL) {
    /* found section with this name */
    free(section);
    return prev;
  }

  /* not found, append */
  section->next = *s;
  *s = section;
  return section;
}
