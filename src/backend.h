#ifndef BACKEND_H_
#define BACKEND_H_

#include "config.h"
#include "log.h"

typedef struct f2b_backend_t {
  void *h;
  bool (*init)  (const char *key, const char *value);
  bool (*ready) (void);
  bool (*ban)   (const char *ip);
  bool (*unban) (const char *ip);
  bool (*exists)(const char *ip);
} f2b_backend_t;

f2b_backend_t * f2b_backend_create (f2b_config_section_t *config);
void            f2b_backend_destroy(f2b_backend_t *backend);

#endif /* BACKEND_H_ */
