#include "common.h"
#include "jail.h"

#define DEFAULT_STATE   true
#define DEFAULT_BANTIME 3600 /* in seconds, 1 hour */
#define DEFAULT_TRIES   5

static f2b_jail_t defaults = {
  .enabled  = DEFAULT_STATE,
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

bool
f2b_jail_ban(f2b_jail_t *jail, f2b_ipaddr_t *addr) {
  assert(jail != NULL);
  assert(addr != NULL);

  addr->matches.used = 0;
  addr->banned  = true;
  addr->bantime = addr->lastseen;

  if (f2b_backend_ban(jail->backend, addr->text)) {
    f2b_log_msg(log_info, "banned ip in jail '%s': %s", jail->name, addr->text);
    return true;
  }

  f2b_log_msg(log_error, "can't ban ip in jail '%s': backend failure for '%s'", jail->name, addr->text);
  return false;
}

bool
f2b_jail_unban(f2b_jail_t *jail, f2b_ipaddr_t *addr) {
  assert(jail != NULL);
  assert(addr != NULL);

  addr->banned  = false;
  addr->bantime = 0;

  if (f2b_backend_unban(jail->backend, addr->text)) {
    f2b_log_msg(log_info, "released ip in jail '%s': %s", jail->name, addr->text);
    return true;
  }

  f2b_log_msg(log_error, "can't release ip in jail '%s': backend failure for '%s'", jail->name, addr->text);
  return false;
}

size_t
f2b_jail_process(f2b_jail_t *jail) {
  f2b_logfile_t *file = NULL;
  f2b_ipaddr_t  *addr = NULL;
  size_t processed = 0;
  char logline[LOGLINE_MAX] = "";
  char matchbuf[IPADDR_MAX] = "";
  time_t now  = time(NULL);
  time_t release_time = 0;

  assert(jail != NULL);

  for (file = jail->logfiles; file != NULL; file = file->next) {
    while (f2b_logfile_getline(file, logline, sizeof(logline))) {
      if (!f2b_regexlist_match(jail->regexps, logline, matchbuf, sizeof(matchbuf)))
        continue;
      /* some regex matches the line */
      addr = f2b_addrlist_lookup(jail->ipaddrs, matchbuf);
      if (!addr) {
        /* new ip */
        addr = f2b_ipaddr_create(matchbuf, jail->tries);
        addr->lastseen = now;
        f2b_matches_append(&addr->matches, now);
        jail->ipaddrs = f2b_addrlist_append(jail->ipaddrs, addr);
        f2b_log_msg(log_debug, "new ip found by jail '%s': %s", jail->name, matchbuf);
        continue;
      }
      /* this ip was seen before */
      addr->lastseen = now;
      if (addr->banned) {
        f2b_log_msg(log_warn, "found ip that was already banned by jail '%s': %s", jail->name, matchbuf);
        continue;
      }
      f2b_matches_expire(&addr->matches, now - jail->bantime);
      f2b_matches_append(&addr->matches, now);
      if (addr->matches.used < jail->tries) {
        f2b_log_msg(log_debug, "new match in jail '%s': %s (%d/%d)", jail->name, matchbuf, addr->matches.used, addr->matches.max);
        continue;
      }
      /* limit reached, ban ip */
      f2b_jail_ban(jail, addr);
     } /* while(lines) */
  } /* for(files) */

  for (addr = jail->ipaddrs; addr != NULL; addr = addr->next) {
    if (!addr->banned)
      continue;
    release_time = addr->bantime + jail->bantime;
    if (now < release_time) {
      f2b_log_msg(log_debug, "skipping banned ip in jail '%s': %s (%.1fh remains)", jail->name, addr->text, (now - release_time) / 3600);
      continue;
    }
    f2b_jail_unban(jail, addr);
  }

  return processed;
}
