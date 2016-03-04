#include "common.h"
#include "jail.h"

#define DEFAULT_BANTIME 3600 /* in seconds, 1 hour */
#define DEFAULT_TRIES   5

static f2b_jail_t defaults = {
  .enabled  = true,
  .bantime  = DEFAULT_BANTIME,
  .tries    = DEFAULT_TRIES,
};

void
f2b_jail_apply_config(f2b_jail_t *jail, f2b_config_section_t *config) {
  f2b_config_param_t *param = NULL;

  assert(jail != NULL);
  assert(config != NULL);
  assert(config->type != t_jail);

  param = config->param;
  for (; param != NULL; param = param->next) {
    if (strcmp(param->name, "enabled") == 0) {
      if (strcmp(param->value, "yes") == 0)
        jail->enabled = true;
      continue;
    }
    if (strcmp(param->name, "bantime") == 0) {
      jail->bantime = atoi(param->value);
      if (jail->bantime <= 0)
        jail->bantime = DEFAULT_BANTIME;
      continue;
    }
    if (strcmp(param->name, "tries") == 0) {
      jail->bantime = atoi(param->value);
      if (jail->tries <= 0)
        jail->tries = DEFAULT_TRIES;
      continue;
    }
    f2b_log_msg(log_warn, "unrecognized param in section [defaults]: %s", param->name);
  }

  return;
}

void
f2b_jail_set_defaults(f2b_config_section_t *config) {
  assert(config != NULL);
  assert(config->type == t_defaults);

  config->type = t_jail;
  f2b_jail_apply_config(&defaults, config);
  config->type = t_defaults;

  return;
}

size_t
f2b_jail_poll(f2b_jail_t *jail) {
  f2b_logfile_t *file = NULL;
  f2b_ipaddr_t  *addr = NULL;
  size_t processed = 0;
  char logline[LOGLINE_MAX] = "";
  char matchbuf[IPADDR_MAX] = "";

  assert(jail != NULL);

  for (file = jail->logfiles; file != NULL; file = file->next) {
    if (!f2b_logfile_getline(file, logline, sizeof(logline)))
      continue;
    if (!f2b_regexlist_match(jail->regexps, logline, matchbuf, sizeof(matchbuf)))
      continue;
    addr = f2b_addrlist_lookup(jail->ipaddrs, matchbuf);
    if (!addr) {
      addr = f2b_ipaddr_create(matchbuf, jail->tries);
      jail->ipaddrs = f2b_addrlist_append(jail->ipaddrs, addr);
      /* TODO: log */
      continue;
    }
    /* TODO: find & handle */
  }

  return processed;
}
