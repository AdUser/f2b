General notes
=============

f2b config consists from main config `/etc/f2b/f2b.conf` and several config parts located in `/etc/f2b/conf-enabled/`.
All config parts should ends with `.conf` and ill be sorted before processing.

Config consists from several sections of various types:

* [main] -- daemon settings
* [defaults] -- jail defaults
* [source:*] -- source module definitions
* [filter:*] -- filter module definitions
* [backend:*] -- backend module definitions
* [jail:*] -- jail definitions

Options may be redefined by creating section with same name and option with changed value. Like this:

    # from f2b.conf
    [main]
    <skipped>
    loglevel = info  # <- you want to redefine this
    # from conf-enabled/99-local.conf
    [main]
    loglevel = error # <- this will be actual value

Also you need to define one or more jails.
Each jail should have at least `source`, `filter` and `backend` options.
Missing options will be inherited from `[default]` section.
Jail will be ignored unless `enabled` option set to `yes`.

Example:

    [jail:test] # <- this jail will be named 'test'
    enabled = yes
    source = files:/var/log/messages
    # search for source named `files` and init it with string '/var/log/messages'
    filter = preg:/etc/f2b/filters/ssh.preg
    # search for filter named 'preg' and init it with path '/etc/f2b/filters/ssh.preg'
    # note '.preg' extension - it's a hint for you,
    # that this file contains regexps compatible with posix syntax
    # see regex(7) manpage
    backend = exec-ipset:banned
    # search for backend named 'exec-ipset' and init it with string 'banned'
    # So <ID> token will be replaced with 'banned' string

Module-specific settings
========================

Each module has so-called "init string" with different syntax for each module.
Example:

    [jail:test]
    <skipped>
    backend = backend_name:init-string

Source-modules
--------------

files
:   Init string is path to file which should be polled for data.
    Also may by file pattern if `glob = yes` is set.
    For pattern syntax see glob(7)

redis
:   Init string is name of pubsub channel on redis server (will be prefixed with "f2b-")
    Options are:
    * server -- ip address for redis server we should connect
    * port -- redis server port (set if non-standart)
    * database -- database number on redis server (see SELECT redis command)
    * password -- password for redis auth after connect (see AUTH redis command)

Filter-modules
--------------

Init string is path to file with patterns list. Each pattern should has exactly one <HOST> token in place of ip address appears.

preg
:   Only option is `icase` - set to 'yes' for case-insensitive pattern matches
    This module has additional restriction. <HOST> token must be placed before first capture group.

pcre
:   Options are:
    * icase -- set to 'yes' for case-insensitive pattern matches
    * study -- if set to 'yes' each pattern consumes more memory, but matches faster
    * usejit -- set to 'yes' to use jit pcre usage (only for pcre >= 8.20)

Backend-modules
---------------

Each backend has at least these options:
* shared  -- is this module may be reused between different jails
* timeout -- timeout for commands ban/check/unban

exec
:   Init string is arbitrary string which may be used in args of executed command(s) with <ID> token.
    Options start/stop/ban/unban/check should contain path to script/binary to execute on named action.
    This options can be used more than once, commands will be executed in order they appers in config.
    Tokens <ID>/<IP> will be replaced with actual values.

redis
:   Init string is name of pubsub channel on redis server (will be prefixed with "f2b-")
    Options almost the same as source/redis. 'ping' option - for keeping connection alive (see PING redis command)
