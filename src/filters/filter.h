#include <stdbool.h>

#define HOST_TOKEN "<HOST>"

typedef struct _config cfg_t;

extern cfg_t *create(const char *id);
extern const char *error(cfg_t *c);
extern bool   config(cfg_t *c, const char *key, const char *value);
extern bool   append(cfg_t *c, const char *pattern);
extern bool    ready(cfg_t *c);
extern bool    match(cfg_t *c, const char *line, char *buf, size_t buf_size);
extern void  destroy(cfg_t *c);
