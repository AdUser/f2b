/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "jail.h"

#define DEFAULT_BANTIME  3600 /* in seconds, 1 hour */
#define DEFAULT_FINDTIME  300 /* in seconds, 5 min */
#define DEFAULT_EXPIRETIME 14400 /* in seconds, 4 hours */
#define DEFAULT_MAXRETRY    5

f2b_jail_t *jails = NULL;

static f2b_jail_t defaults = {
  .bantime  = DEFAULT_BANTIME,
  .findtime = DEFAULT_FINDTIME,
  .maxretry = DEFAULT_MAXRETRY,
};

void
f2b_jail_parse_compound_value(const char *value, char *name, char *init) {
  size_t len = 0;
  char *p = NULL;

  if ((p = strchr(value, ':')) == NULL) {
    /* param = name */
    strlcpy(name, value, CONFIG_KEY_MAX);
    return;
  }

  /* param = name:init_string */
  len = p - value;
  if (len >= CONFIG_KEY_MAX) {
    f2b_log_msg(log_warn, "'name' part of value exceeds max length %d bytes: %s", CONFIG_KEY_MAX, value);
    return;
  }

  strlcpy(name, value, len + 1); /* ':' acts as '\0' */
  strlcpy(init, (p + 1), CONFIG_VAL_MAX);
  return;
}

bool
f2b_jail_set_param(f2b_jail_t *jail, const char *param, const char *value) {
  assert(jail != NULL);
  assert(param != NULL);
  assert(value != NULL);

  if (strcmp(param, "enabled") == 0) {
    if (strcmp(value, "yes") == 0) {
      jail->flags |= JAIL_ENABLED;
    } else {
      jail->flags &= ~JAIL_ENABLED;
    }
    return true;
  }
  if (strcmp(param, "bantime") == 0) {
    jail->bantime = atoi(value);
    if (jail->bantime <= 0)
      jail->bantime = DEFAULT_BANTIME;
    return true;
  }
  if (strcmp(param, "findtime") == 0) {
    jail->findtime = atoi(value);
    if (jail->findtime <= 0)
      jail->findtime = DEFAULT_FINDTIME;
    return true;
  }
  if (strcmp(param, "expiretime") == 0) {
    jail->expiretime = atoi(value);
    if (jail->expiretime <= 0)
      jail->expiretime = DEFAULT_EXPIRETIME;
    return true;
  }
  if (strcmp(param, "maxretry") == 0) {
    jail->maxretry = atoi(value);
    if (jail->maxretry == 0)
      jail->maxretry = DEFAULT_MAXRETRY;
    return true;
  }
  if (strcmp(param, "incr_bantime") == 0) {
    jail->incr_bantime = atof(value);
    return true;
  }
  if (strcmp(param, "incr_findtime") == 0) {
    jail->incr_findtime = atof(value);
    return true;
  }
  return false;
}

void
f2b_jail_apply_config(f2b_jail_t *jail, f2b_config_section_t *section) {
  f2b_config_param_t *param = NULL;

  assert(jail != NULL);
  assert(section != NULL);
  assert(section->type == t_jail || section->type == t_defaults);

  for (param = section->param; param != NULL; param = param->next) {
    if (strcmp(param->name, "source") == 0) {
      f2b_jail_parse_compound_value(param->value, jail->source_name, jail->source_init);
      continue;
    }
    if (strcmp(param->name, "filter") == 0) {
      f2b_jail_parse_compound_value(param->value, jail->filter_name, jail->filter_init);
      continue;
    }
    if (strcmp(param->name, "backend") == 0) {
      f2b_jail_parse_compound_value(param->value, jail->backend_name, jail->backend_init);
      continue;
    }
    if (strcmp(param->name, "state") == 0) {
      if (strcmp(param->value, "yes") == 0) {
        jail->flags |= JAIL_HAS_STATE;
      } else {
        jail->flags &= ~JAIL_HAS_STATE;
      }
      continue;
    }
    if (f2b_jail_set_param(jail, param->name, param->value))
      continue;
    f2b_log_msg(log_warn, "jail '%s': unrecognized parameter: %s", jail->name, param->name);
  }

  return;
}

void
f2b_jail_set_defaults(f2b_config_section_t *section) {
  assert(section != NULL);
  assert(section->type == t_defaults);

  f2b_jail_apply_config(&defaults, section);

  return;
}

bool
f2b_jail_ban(f2b_jail_t *jail, f2b_ipaddr_t *addr) {
  time_t bantime = 0;
  assert(jail != NULL);
  assert(addr != NULL);

  addr->matches.hits = 0;
  addr->matches.used = 0;
  addr->banned  = true;
  addr->banned_at = addr->lastseen;
  if (jail->incr_bantime > 0) {
    bantime = jail->bantime + (int) (addr->bancount * (jail->bantime * jail->incr_bantime));
  } else {
    bantime = jail->bantime;
  }
  addr->bancount++;
  addr->release_at = addr->banned_at + bantime;
  jail->bancount++;

  if (f2b_backend_check(jail->backend, addr->text)) {
    f2b_log_msg(log_warn, "jail '%s': ip %s was already banned", jail->name, addr->text);
    return true;
  }

  if (f2b_backend_ban(jail->backend, addr->text)) {
    f2b_log_msg(log_note, "jail '%s': banned ip %s for %.1fhrs",
      jail->name, addr->text, (float) bantime / 3600);
    return true;
  }

  f2b_log_msg(log_error, "jail '%s': can't ban ip %s -- %s",
    jail->name, addr->text, f2b_backend_error(jail->backend));
  return false;
}

bool
f2b_jail_unban(f2b_jail_t *jail, f2b_ipaddr_t *addr) {
  assert(jail != NULL);
  assert(addr != NULL);

  addr->banned  = false;
  addr->banned_at = 0;
  addr->release_at = 0;

  if (f2b_backend_unban(jail->backend, addr->text)) {
    f2b_log_msg(log_note, "jail '%s': released ip %s", jail->name, addr->text);
    return true;
  }

  f2b_log_msg(log_error, "jail '%s': can't release ip %s -- %s",
    jail->name, addr->text, f2b_backend_error(jail->backend));
  return false;
}

f2b_jail_t *
f2b_jail_create(f2b_config_section_t *section) {
  f2b_jail_t *jail = NULL;

  assert(section != NULL);
  assert(section->type == t_jail);

  if ((jail = calloc(1, sizeof(f2b_jail_t))) == NULL) {
    f2b_log_msg(log_error, "calloc() for new jail failed");
    return NULL;
  }

  memcpy(jail, &defaults, sizeof(f2b_jail_t));
  strlcpy(jail->name, section->name, sizeof(jail->name));
  f2b_jail_apply_config(jail, section);

  return jail;
}

f2b_jail_t *
f2b_jail_find(f2b_jail_t *list, const char *name) {
  assert(name != NULL);

  for (; list != NULL; list = list->next)
    if (strcmp(list->name, name) == 0)
      return list;

  return NULL;
}

size_t
f2b_jail_process(f2b_jail_t *jail) {
  f2b_ipaddr_t  *prev = NULL;
  f2b_ipaddr_t  *addr = NULL;
  size_t processed = 0;
  char line[LOGLINE_MAX] = "";
  char matchbuf[IPADDR_MAX] = "";
  time_t now  = time(NULL);
  time_t findtime = 0;
  time_t expiretime = 0;
  bool remove = false;
  bool reset = true; /* source reset */

  assert(jail != NULL);

  f2b_log_msg(log_debug, "jail '%s': processing", jail->name);

  f2b_backend_ping(jail->backend);

  while (f2b_source_next(jail->source, line, sizeof(line), reset)) {
    reset = false;
    if (!f2b_filter_match(jail->filter, line, matchbuf, sizeof(matchbuf)))
      continue;
    /* some regex matches the line */
    jail->matchcount++;
    addr = f2b_addrlist_lookup(jail->ipaddrs, matchbuf);
    if (!addr) {
      addr = f2b_ipaddr_create(matchbuf, jail->maxretry);
      jail->ipaddrs = f2b_addrlist_append(jail->ipaddrs, addr);
      f2b_log_msg(log_debug, "jail '%s': new ip found -- %s", jail->name, matchbuf);
    }
    addr->lastseen = now;
    if (addr->banned) {
      if (addr->banned_at != now)
        f2b_log_msg(log_warn, "jail '%s': ip %s was already banned", jail->name, matchbuf);
      continue;
    }
    if (jail->incr_findtime > 0 && addr->matches.hits > jail->maxretry) {
      findtime = now - jail->findtime;
      findtime -= (int) ((addr->matches.hits - jail->maxretry) *
                         (jail->findtime * jail->incr_findtime));
    } else {
      findtime = now - jail->findtime;
    }
    f2b_matches_expire(&addr->matches, findtime);
    f2b_matches_append(&addr->matches, now);
    if (addr->matches.used < jail->maxretry) {
      f2b_log_msg(log_info, "jail '%s': new ip match -- %s (%zu/%zu)",
        jail->name, matchbuf, addr->matches.used, addr->matches.max);
      continue;
    }
    /* limit reached, ban ip */
    f2b_jail_ban(jail, addr);
  }  /* while(1) */

  for (addr = jail->ipaddrs, prev = NULL; addr != NULL; ) {
    remove = false;
    /* check release time */
    if (addr->banned && now > addr->release_at)
      f2b_jail_unban(jail, addr);
    /* check expiration */
    expiretime = (addr->lastseen >= addr->release_at)
      ? addr->lastseen
      : addr->release_at;
    expiretime += jail->expiretime;
    if (now > expiretime) {
      f2b_log_msg(log_info, "jail '%s': expired ip -- %s",
        jail->name, addr->text);
      remove = true;
    }
    /* list cleanup */
    if (!remove) {
      prev = addr, addr = addr->next;
      continue;
    }
    /* remove from list */
    if (prev == NULL) {
      /* first item in list */
      jail->ipaddrs = addr->next;
      f2b_ipaddr_destroy(addr);
      addr = jail->ipaddrs;
    } else {
      /* somewhere in list */
      prev->next = addr->next;
      f2b_ipaddr_destroy(addr);
      addr = prev->next;
    }
  }

  return processed;
}

bool
f2b_jail_init(f2b_jail_t *jail, f2b_config_t *config) {
  f2b_config_section_t * s_section = NULL;
  f2b_config_section_t * b_section = NULL;
  f2b_config_section_t * f_section = NULL;

  assert(jail   != NULL);
  assert(config != NULL);

  /* source */
  if (jail->source_name[0] == '\0') {
    f2b_log_msg(log_error, "jail '%s': missing 'source' parameter", jail->name);
    return false;
  }
  if ((s_section = f2b_config_section_find(config->sources, jail->source_name)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no source with name '%s'", jail->name, jail->source_name);
    return false;
  }

  /* filter */
  if (jail->filter_name[0] == '\0') {
    f2b_log_msg(log_error, "jail '%s': missing 'filter' parameter", jail->name);
    return false;
  }
  if ((f_section = f2b_config_section_find(config->filters, jail->filter_name)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no filter with name '%s'", jail->name, jail->filter_name);
    return false;
  }

  /* backend */
  if (jail->backend_name[0] == '\0') {
    f2b_log_msg(log_error, "jail '%s': missing 'backend' parameter", jail->name);
    return false;
  }
  if ((b_section = f2b_config_section_find(config->backends, jail->backend_name)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no backend with name '%s'", jail->name, jail->backend_name);
    return false;
  }

  /* init all */
  if ((jail->source = f2b_source_create(s_section, jail->source_init, f2b_log_error_cb)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no regexps loaded from '%s'", jail->name, jail->source_init);
    goto cleanup;
  }
  if (!f2b_source_start(jail->source)) {
    f2b_log_msg(log_warn, "jail '%s': source action 'start' failed -- %s",
      jail->name, f2b_source_error(jail->source));
    goto cleanup;
  }

  if ((jail->filter = f2b_filter_create(f_section, jail->filter_init)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no regexps loaded from '%s'", jail->name, jail->filter_init);
    goto cleanup;
  }

  if ((jail->backend = f2b_backend_create(b_section, jail->backend_init)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': can't init backend '%s' with %s",
      jail->name, jail->backend_name, jail->backend_init);
    goto cleanup;
  }
  if (!f2b_backend_start(jail->backend)) {
    f2b_log_msg(log_warn, "jail '%s': backend action 'start' failed -- %s",
      jail->name, f2b_backend_error(jail->backend));
    goto cleanup;
  }

  f2b_log_msg(log_debug, "jail '%s' init complete", jail->name);

  return true;

  cleanup:
  if (jail->source)
    f2b_source_destroy(jail->source);
  if (jail->filter)
    f2b_filter_destroy(jail->filter);
  if (jail->backend)
    f2b_backend_destroy(jail->backend);
  return false;
}

bool
f2b_jail_start(f2b_jail_t *jail) {
  time_t now = time(NULL);
  time_t remains;

  assert(jail != NULL);

  if (jail->flags & JAIL_HAS_STATE) {
    jail->sfile = f2b_statefile_create(jail->name, appconfig.statedir_path);
    if (jail->sfile == NULL) {
      /* error occured, must be already logged, just drop flag */
      jail->flags &= ~JAIL_HAS_STATE;
    } else {
      jail->ipaddrs = f2b_statefile_load(jail->sfile, jail->maxretry);
    }
  }

  for (f2b_ipaddr_t *addr = jail->ipaddrs; addr != NULL; addr = addr->next) {
    if (!addr->banned)
      continue; /* if list NOW contains such addresses, it may be bug */
    if (f2b_backend_check(jail->backend, addr->text))
      continue; /* already banned or backend don't support check() */
    if (now >= addr->release_at) {
      addr->banned = false;
      continue; /* ban time already expired */
    }
    if (f2b_backend_ban(jail->backend, addr->text)) {
      remains = addr->release_at - now;
      f2b_log_msg(log_note, "jail '%s': restored ban of ip %s (%.1fhrs remain)",
        jail->name, addr->text, (float) remains / 3600);
    } else {
      f2b_log_msg(log_error, "jail '%s': can't ban ip %s -- %s",
        jail->name, addr->text, f2b_backend_error(jail->backend));
    }
  }

  f2b_log_msg(log_info, "jail '%s' started", jail->name);

  return true;
}

bool
f2b_jail_stop(f2b_jail_t *jail) {
  bool errors = false;

  assert(jail != NULL);

  f2b_log_msg(log_info, "jail '%s': gracefull shutdown", jail->name);

  if (!f2b_source_stop(jail->source)) {
    f2b_log_msg(log_error, "jail '%s': action 'stop' for source failed: %s",
      jail->name, f2b_source_error(jail->source));
    errors = true;
  }

  f2b_source_destroy(jail->source);
  f2b_filter_destroy(jail->filter);

  for (f2b_ipaddr_t *addr = jail->ipaddrs; addr != NULL; addr = addr->next) {
    if (!addr->banned)
      continue;
    if (f2b_jail_unban(jail, addr))
      continue;
    errors = true;
  }
  f2b_addrlist_destroy(jail->ipaddrs);

  if (!f2b_backend_stop(jail->backend)) {
    f2b_log_msg(log_error, "jail '%s': action 'stop' for backend failed: %s",
      jail->name, f2b_backend_error(jail->backend));
    errors = true;
  }

  return errors;
}

void
f2b_jail_cmd_status(char *res, size_t ressize, f2b_jail_t *jail) {
  const char *fmt =
    "name: %s\n"
    "flags:\n"
    "  enabled: %s\n"
    "  state: %s\n"
    "maxretry: %d\n"
    "times:\n"
    "  bantime: %d\n"
    "  findtime: %d\n"
    "  expiretime: %d\n"
    "incr:\n"
    "  bantime: %.1f\n"
    "  findtime: %.1f\n"
    "stats:\n"
    "  banned: %d\n"
    "  matched: %d\n";

  assert(res  != NULL);
  assert(jail != NULL);

  snprintf(res, ressize, fmt, jail->name,
    jail->flags & JAIL_ENABLED     ? "yes" : "no",
    jail->flags & JAIL_HAS_STATE   ? "yes" : "no",
    jail->maxretry,
    jail->bantime, jail->findtime, jail->expiretime,
    jail->incr_bantime, jail->incr_findtime,
    jail->bancount, jail->matchcount);
}

void
f2b_jail_cmd_set(char *res, size_t ressize, f2b_jail_t *jail, const char *param, const char *value) {
  assert(res   != NULL);
  assert(jail  != NULL);
  assert(param != NULL);
  assert(value != NULL);

  if (f2b_jail_set_param(jail, param, value))
    return;
  snprintf(res, ressize, "parameter not found: %s", param);
}

/**
 * @brief misc operations on ip in given jail
 * @param res  response buffer (don't change if no error)
 * @param ressize response buffer size
 * @param jail selected jail
 * @param op Type of operation: >0 - ban, 0 - status, <0 - unban
 * @param ip IP address
 */
void
f2b_jail_cmd_ip_xxx(char *res, size_t ressize, f2b_jail_t *jail, int op, const char *ip) {
  f2b_ipaddr_t *addr = NULL;

  assert(res  != NULL);
  assert(jail != NULL);
  assert(ip   != NULL);

  if ((addr = f2b_addrlist_lookup(jail->ipaddrs, ip)) == NULL) {
    /* address not found in list */
    if (op > 0) {
      /* ban */
      time_t now = time(NULL);
      addr = f2b_ipaddr_create(ip, jail->maxretry);
      if (!addr) {
        snprintf(res, ressize, "can't parse ip address: %s", ip);
        return;
      }
      addr->lastseen = now;
      f2b_matches_append(&addr->matches, now);
      jail->ipaddrs = f2b_addrlist_append(jail->ipaddrs, addr);
    } else {
      /* unban & status */
      snprintf(res, ressize, "can't find ip '%s' in jail '%s'", ip, jail->name);
      return;
    }
  }

  if (op > 0) {
    f2b_jail_ban(jail, addr);
  } else if (op < 0) {
    f2b_jail_unban(jail, addr);
  } else {
    f2b_ipaddr_status(addr, res, ressize);
  }
}
