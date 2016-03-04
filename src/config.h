#ifndef CONFIG_H_
#define CONFIG_H_

#define CONFIG_LINE_MAX 256

#define CONFIG_KEY_MAX 32
#define CONFIG_VAL_MAX 192

typedef enum f2b_section_type {
  t_unknown = 0,
  t_main,
  t_defaults,
  t_backend,
  t_jail,
} f2b_section_type;

typedef struct f2b_config_param_t {
  struct f2b_config_param_t *next;
  char name[CONFIG_KEY_MAX + 1];
  char value[CONFIG_VAL_MAX + 1];
} f2b_config_param_t;

typedef struct f2b_config_section_t {
  struct f2b_config_section_t *next;
  char name[CONFIG_KEY_MAX];
  f2b_section_type type;
  f2b_config_param_t *param;
  f2b_config_param_t *last;
} f2b_config_section_t;

f2b_config_param_t * f2b_config_parse_kv_pair(const char *line);
f2b_config_param_t * f2b_config_find_param(f2b_config_param_t *param, const char *name);

f2b_config_section_t * f2b_config_parse_section(const char *line);
f2b_config_section_t * f2b_config_find_section(f2b_config_section_t *section, f2b_section_type type, const char *name);
f2b_config_section_t * f2b_config_append_param(f2b_config_section_t *section, f2b_config_param_t *param);

f2b_config_section_t * f2b_config_load(const char *path);
void                   f2b_config_free(f2b_config_section_t *config);
#endif /* CONFIG_H_ */
