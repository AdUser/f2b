#include "../src/common.h"
#include "../src/config.h"

int main() {
  f2b_config_param_t *param = NULL;

  assert(f2b_config_parse_kv_pair("") == NULL);
  assert(f2b_config_parse_kv_pair("#") == NULL);
  assert(f2b_config_parse_kv_pair("=") == NULL);
  assert(f2b_config_parse_kv_pair("key=") == NULL);
  assert(f2b_config_parse_kv_pair("key =") == NULL);
  assert(f2b_config_parse_kv_pair("key = ") == NULL);
  assert(f2b_config_parse_kv_pair(  "=value") == NULL);
  assert(f2b_config_parse_kv_pair( "= value") == NULL);
  assert(f2b_config_parse_kv_pair(" = value") == NULL);

  assert((param = f2b_config_parse_kv_pair("key=value")) != NULL);
  assert(strcmp(param->name,  "key")   == 0);
  assert(strcmp(param->value, "value") == 0);
  free(param);

  assert((param = f2b_config_parse_kv_pair("key = value")) != NULL);
  assert(strcmp(param->name,  "key")   == 0);
  assert(strcmp(param->value, "value") == 0);
  free(param);

  assert((param = f2b_config_parse_kv_pair("key=value #comment")) != NULL);
  assert(strcmp(param->name,  "key")   == 0);
  assert(strcmp(param->value, "value") == 0);
  free(param);

  assert((param = f2b_config_parse_kv_pair("key=value#compose")) != NULL);
  assert(strcmp(param->name,  "key")   == 0);
  assert(strcmp(param->value, "value#compose") == 0);
  free(param);

  return 0;
}
