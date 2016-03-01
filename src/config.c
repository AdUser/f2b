#include "common.h"
#include "config.h"

f2b_config_param_t *
f2b_config_parse_kv_pair(char *line) {
  f2b_config_param_t *param = NULL;
  char *p, *key, *value;

  key = line;

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
  if ((p = strstr(value, " #")) || (p = strstr(value, "\t#")))
    *p = '\0';

  /* strip trailing spaces */
  p = value + strlen(value);
  if (p > value)
    p--; /* step back at char before '\0' */
  while (p > value && isblank(*p))
    p--;
  p++, *p = '\0';

  if (strlen(key) > CONFIG_KEY_MAX || strlen(value) > CONFIG_VAL_MAX)
    return NULL;

  if ((param = calloc(1, sizeof(f2b_config_param_t))) == NULL) {
    strncpy(param->name,  key,   sizeof(param->name));
    strncpy(param->value, value, sizeof(param->value));
    param->name[CONFIG_KEY_MAX] = '\0';
    param->name[CONFIG_VAL_MAX] = '\0';
    return param;
  }

  return NULL;
}

f2b_config_section_t *
f2b_config_parse_section(char *line) {
  f2b_config_section_t *section = NULL;
  char *name, *end;

  if ((end = strchr(line, ']')) == NULL)
    return NULL;
  *end = '\0';

  if ((section = calloc(1, sizeof(f2b_config_section_t))) == NULL)
    return NULL;

  name = "[defaults]";
  if (strncmp(line, name, strlen(name)) == 0) {
    section->type = t_defaults;
    return section;
  }

  name = "[backend:";
  if (strncmp(line, name, strlen(name)) == 0) {
    section->type = t_backend;
    strncpy(section->name, line + strlen(name), sizeof(section->name));
    return section;
  }

  name = "[jail:";
  if (strncmp(line, name, strlen(name)) == 0) {
    section->type = t_jail;
    strncpy(section->name, line + strlen(name), sizeof(section->name));
    return section;
  }

  free(section);
  return NULL;
}

f2b_config_section_t *
f2b_config_find_section(f2b_config_section_t *config, f2b_section_type type, const char *name) {
  for (; config != NULL; config = config->next) {
    if (config->type == type && strcmp(config->name, name) == 0)
      return config;
  }

  return NULL;
}

void
f2b_config_free(f2b_config_section_t *config) {
  f2b_config_section_t *next_section = NULL;
  f2b_config_param_t   *next_param   = NULL;

  for (; config != NULL; config = next_section) {
    next_section = config->next;
    for (; config->param != NULL; config->param = next_param) {
      next_param = config->param->next;
      FREE(config->param);
    }
    FREE(next_section);
  }
}
