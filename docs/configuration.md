General notes
=============

f2b config consists from main config `/etc/f2b/f2b.conf` and several config parts located in `/etc/f2b/conf-enabled/`.
All config parts should ends with `.conf` and will be sorted before processing.

Config consists from several sections of various types:

* [main] -- daemon settings
* [defaults] -- jail defaults
* [source:%%] -- source module definitions
* [filter:%%] -- filter module definitions
* [backend:%%] -- backend module definitions
* [jail:%%] -- jail definitions

Options may be redefined by creating section with same name and option with changed value. Like this:

    # from f2b.conf
    [main]
    <skipped>
    loglevel = info  # <- you may want to redefine this
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
    # that this file contains regexps compatible with POSIX regex syntax
    # see regex(7) manpage
    backend = exec-ipset:banned
    # search for backend named 'exec-ipset' and init it with string 'banned'
    # So <ID> token will be replaced with 'banned' string

See *Teamwork* section for advanced samples.

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

portknock
:   Init string is ignored. Only option is `listen` -- set address/port for tcp honeypot.
    Address is optional, eg "23" and "0.0.0.0:23" is the same.
    For ipv6 address use square brackets like this: "[fe80::fe14:a87c]:23".


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

ipset
:   Init string is ipset set name. The set should already exist, this module only add/del/check addresses.
    Set creation/destroying is planned but not implemented yet.

redis
:   Init string is name of pubsub channel on redis server (will be prefixed with "f2b-")
    Options almost the same as source/redis. 'ping' option - for keeping connection alive (see PING redis command)

Teamwork
========

This section shows sample configurations of distributed f2b installations.

In standalone install you have simple workflow:

  * `jail/source`  gets the data
  * `jail/filter`  decides is we need this data piece
  * `jail/matches` decides should we ban this ip or not
  * `jail/backend` directly bans/releases filtered IPs

Now let's change the workflow:

  * "sensor" jail
    * `jail:sensor/source` gets the data (same as above)
    * `jail:sensor/filter` decides is we need this data piece (same as above)
    * `jail:sensor/matches` decides should we ban this ip or not
    * `jail:sensor/backend`  sends filtered ip to some blackbox
  * "actor" jail
    * `jail:actor/source` gets the data from blackbox
    * `jail:actor/filter` only checks is we get valid ip
    * `jail:actor/matches` decides should we ban this ip or not
    * `jail:actor/backend` bans/releases given IPs

For now supported `blackbox` type is `redis database`.

Various jail types may be combined and omitted. Some possible variants:

    [   HOST1   ]    [ HOST2 ]     [   HOST3   ]
    jail:sensor1 --> [-------] <-- jail:sensor3
    jail:sensor2 --> [ REDIS ] <-- jail:sensor4
    jail:actor1  <-- [  DB   ] --> jail:actor2

Now let's see real configs. This is modified sample from section `General notes`.

    [source:redis]
    server = 127.0.0.1
    ;
    [backend:redis]
    server = 127.0.0.1
    ;
    [jail:sensor1]
    enabled = yes
    source = files:/var/log/messages
    filter = preg:/etc/f2b/filters/ssh.preg
    backend = redis:ssh
    ;
    [jail:actor1]
    enabled = yes
    source = redis:ssh
    filter = preg:/etc/f2b/filters/empty.preg
    backend = exec-ipset:banned

Now, if "sensor1" detects some malicious activity it sends notify with redis PUBLISH command on channel f2b-banned-ssh.
You may see this messages with the following commands:

    # at host1
    $ redis-cli -h 127.0.0.1
    127.0.0.1:6379> SUBSCRIBE f2b-banned-ssh # <- "ssh" here is taken from init string of sensor1/source
    ^C
    $
    $ redis-cli -h 127.0.0.1
    127.0.0.1:6379> PUBLISH f2b-banned-ssh 1.2.3.4 # <- manually ban given ip

!!! Important note

This configuration for now is one-way street: it only distribute ban events, not release events.
This means, if you execute the last command above, this ip will be banned on all configured f2b instance as quick as possible.
But if you want to unban it, you'll need either wait for $bantime second(s) or manually release it at each configured f2b instance.

This may change in future.
