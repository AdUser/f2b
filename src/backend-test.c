#include "common.h"
#include "config.h"
#include "log.h"
#include "backend.h"

void usage() {
  fprintf(stderr, "Usage: backend-test <config-file.conf> <id>\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  const char *ip = "127.0.0.17";
  f2b_config_section_t *config  = NULL;
  f2b_config_section_t *b_conf  = NULL;
  f2b_backend_t        *backend = NULL;

  if (argc < 3)
    usage();

  if ((config = f2b_config_load(argv[1])) == NULL) {
    f2b_log_msg(log_error, "can't load config");
    return EXIT_FAILURE;
  }

  if ((b_conf = f2b_config_find_section(config, t_backend, "test")) == NULL) {
    f2b_log_msg(log_error, "can't find config section for backend '%s'", "test");
    return EXIT_FAILURE;
  }

  if ((backend = f2b_backend_create(config, argv[2])) == NULL) {
    f2b_log_msg(log_error, "can't create backend");
    return EXIT_FAILURE;
  }

  if (!backend->ban(backend->cfg, ip)) {
    f2b_log_msg(log_error, "action 'ban' failed");
    goto cleanup;
  }

  if (!backend->exists(backend->cfg, ip)) {
    f2b_log_msg(log_error, "action 'exists' failed");
    goto cleanup;
  }

  if (!backend->unban(backend->cfg, ip)) {
    f2b_log_msg(log_error, "action 'unban' failed");
    goto cleanup;
  }

  f2b_log_msg(log_info, "all tests passed");

  cleanup:
  f2b_backend_destroy(backend);
  f2b_config_free(config);

  return EXIT_SUCCESS;
}
