#ifndef CONFIG_H_
#define CONFIG_H_

#define CONFIG_KEY_MAX 32
#define CONFIG_VAL_MAX 192

typedef enum f2b_section_type {
  t_unknown = 0,
  t_global,
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
} f2b_config_section_t;

#endif /* CONFIG_H_ */
