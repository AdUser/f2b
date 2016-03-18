#include "../src/common.h"
#include "../src/config.h"

int main() {
  f2b_config_param_t *param = NULL;

  assert(param == NULL);
  f2b_config_param_create("");
  assert(param == NULL);
  f2b_config_param_create("#");
  assert(param == NULL);
  f2b_config_param_create("=");
  assert(param == NULL);
  f2b_config_param_create("key=");
  assert(param == NULL);
  f2b_config_param_create("key =");
  assert(param == NULL);
  f2b_config_param_create("key = ");
  assert(param == NULL);
  f2b_config_param_create(  "=value");
  assert(param == NULL);
  f2b_config_param_create( "= value");
  assert(param == NULL);
  f2b_config_param_create(" = value");
  assert(param == NULL);

  param = f2b_config_param_create("key=value");
  assert(param != NULL);
  assert(strcmp(param->name,  "key")   == 0);
  assert(strcmp(param->value, "value") == 0);
  free(param);

  param = f2b_config_param_create("key = value");
  assert(param != NULL);
  assert(strcmp(param->name,  "key")   == 0);
  assert(strcmp(param->value, "value") == 0);
  free(param);

  param = f2b_config_param_create("key=value #comment");
  assert(param != NULL);
  assert(strcmp(param->name,  "key")   == 0);
  assert(strcmp(param->value, "value") == 0);
  free(param);

  param = f2b_config_param_create("key=value#compose");
  assert(param != NULL);
  assert(strcmp(param->name,  "key")   == 0);
  assert(strcmp(param->value, "value#compose") == 0);
  free(param);

  return 0;
}
