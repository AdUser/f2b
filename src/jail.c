/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "jail.h"

#define DEFAULT_STATE    false
#define DEFAULT_BANTIME  3600 /* in seconds, 1 hour */
#define DEFAULT_FINDTIME  300 /* in seconds, 5 min */
#define DEFAULT_EXPIRETIME 14400 /* in seconds, 4 hours */
#define DEFAULT_MAXRETRY    5

static f2b_jail_t defaults = {
  .enabled  = DEFAULT_STATE,
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

  strlcpy(name, value, len);
  strlcpy(init, (p + 1), CONFIG_VAL_MAX);
  return;
}

void
f2b_jail_apply_config(f2b_jail_t *jail, f2b_config_section_t *section) {
  f2b_config_param_t *param = NULL;

  assert(jail != NULL);
  assert(section != NULL);
  assert(section->type == t_jail || section->type == t_defaults);

  for (param = section->param; param != NULL; param = param->next) {
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
    if (strcmp(param->name, "findtime") == 0) {
      jail->findtime = atoi(param->value);
      if (jail->findtime <= 0)
        jail->findtime = DEFAULT_FINDTIME;
      continue;
    }
    if (strcmp(param->name, "expiretime") == 0) {
      jail->expiretime = atoi(param->value);
      if (jail->expiretime <= 0)
        jail->expiretime = DEFAULT_EXPIRETIME;
      continue;
    }
    if (strcmp(param->name, "maxretry") == 0) {
      jail->maxretry = atoi(param->value);
      if (jail->maxretry <= 0)
        jail->maxretry = DEFAULT_MAXRETRY;
      continue;
    }
    if (strcmp(param->name, "incr_bantime") == 0) {
      jail->incr_bantime = atof(param->value);
      continue;
    }
    if (strcmp(param->name, "incr_findtime") == 0) {
      jail->incr_findtime = atof(param->value);
      continue;
    }
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

size_t
f2b_jail_process(f2b_jail_t *jail) {
  f2b_logfile_t *file = NULL;
  f2b_ipaddr_t  *prev = NULL;
  f2b_ipaddr_t  *addr = NULL;
  size_t processed = 0;
  char logline[LOGLINE_MAX] = "";
  char matchbuf[IPADDR_MAX] = "";
  time_t now  = time(NULL);
  time_t findtime = 0;
  time_t expiretime = 0;
  bool remove = false;

  assert(jail != NULL);

  f2b_log_msg(log_debug, "jail '%s': processing", jail->name);

  f2b_backend_ping(jail->backend);

  for (file = jail->logfiles; file != NULL; file = file->next) {
    while (f2b_logfile_getline(file, logline, sizeof(logline))) {
      if (!f2b_filter_match(jail->filter, logline, matchbuf, sizeof(matchbuf)))
        continue;
      /* some regex matches the line */
      jail->matchcount++;
      addr = f2b_addrlist_lookup(jail->ipaddrs, matchbuf);
      if (!addr) {
        /* new ip */
        addr = f2b_ipaddr_create(matchbuf, jail->maxretry);
        addr->lastseen = now;
        f2b_matches_append(&addr->matches, now);
        jail->ipaddrs = f2b_addrlist_append(jail->ipaddrs, addr);
        f2b_log_msg(log_info, "jail '%s': new ip found -- %s", jail->name, matchbuf);
        continue;
      }
      /* this ip was seen before */
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
     } /* while(lines) */
  } /* for(files) */

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
  f2b_config_section_t * b_section = NULL;
  f2b_config_section_t * f_section = NULL;

  assert(jail   != NULL);
  assert(config != NULL);

  /* source */
  if (jail->source_name[0] == '\0') {
    f2b_log_msg(log_error, "jail '%s': missing 'source' parameter", jail->name);
    return false;
  }
  /* TODO: temp stub */
  if (strcmp(jail->source_name, "files") != 0) {
    f2b_log_msg(log_error, "jail '%s': 'source' supports only 'files' for now", jail->name);
    return false;
  }
  if (jail->source_init[0] == '\0') {
    f2b_log_msg(log_error, "jail '%s': 'source' requires file or files pattern", jail->name);
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
  if ((jail->logfiles = f2b_filelist_from_glob(jail->source_init)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no files matching '%s' pattern", jail->name, jail->source_init);
    goto cleanup;
  }

  if ((jail->filter = f2b_filter_create(f_section, jail->filter_init)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no regexps loaded from '%s'", jail->name, jail->filter_init);
    goto cleanup;
  }

  if ((jail->backend = f2b_backend_create(b_section, jail->backend_init)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': can't init backend '%s' with %s -- %s",
      jail->name, jail->backend_name, jail->backend_init, f2b_backend_error(jail->backend));
    goto cleanup;
  }
  if (!f2b_backend_start(jail->backend)) {
    f2b_log_msg(log_warn, "jail '%s': backend action 'start' failed -- %s",
      jail->name, f2b_backend_error(jail->backend));
  }

  f2b_log_msg(log_info, "jail '%s': started", jail->name);

  return true;

  cleanup:
  if (jail->logfiles)
    f2b_filelist_destroy(jail->logfiles);
  if (jail->filter)
    f2b_filter_destroy(jail->filter);
  if (jail->backend)
    f2b_backend_destroy(jail->backend);
  return false;
}

bool
f2b_jail_stop(f2b_jail_t *jail) {
  bool errors = false;

  assert(jail != NULL);

  f2b_log_msg(log_info, "jail '%s': gracefull shutdown", jail->name);

  f2b_filelist_destroy(jail->logfiles);
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
    f2b_log_msg(log_error, "jail '%s': action 'stop' failed: %s",
      jail->name, f2b_backend_error(jail->backend));
    errors = true;
  }

  return errors;
}
