#ifndef CONFIG_H_
#define CONFIG_H_

#define CONFIG_LINE_MAX 256

#define CONFIG_KEY_MAX 32
#define CONFIG_VAL_MAX 192

typedef enum f2b_section_type {
  t_unknown = 0,
  t_main,
  t_defaults,
  t_filter,
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

typedef struct f2b_config_t {
  f2b_config_section_t *main;
  f2b_config_section_t *defaults;
  f2b_config_section_t *filters;
  f2b_config_section_t *backends;
  f2b_config_section_t *jails;
} f2b_config_t;

f2b_config_param_t * f2b_config_param_create(const char *line);
f2b_config_param_t * f2b_config_param_find  (f2b_config_param_t *param, const char *name);
f2b_config_param_t * f2b_config_param_append(f2b_config_param_t *list, f2b_config_param_t *p, bool replace);

f2b_config_section_t * f2b_config_section_create(const char *line);
f2b_config_section_t * f2b_config_section_find  (f2b_config_section_t *s, const char *name);

bool                   f2b_config_load  (f2b_config_t *c, const char *path);
void                   f2b_config_free  (f2b_config_t *c);
f2b_config_section_t * f2b_config_append(f2b_config_t *c, f2b_config_section_t *s);
#endif /* CONFIG_H_ */
