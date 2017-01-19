Overview
--------

f2b is lightweight automatic anti-bot turret for your public serivces.

Features:

* written in pure C
* small memory footprint
* minimum dependencies (required: libc, libdl; optional: pcre, redis)
* fully modular: pluggable sources/filters/backends (you may easy write custom one)
* support for distributed installs (teamwork)
* stateful (can save/restore banned ips on restart)
* adapting to bots (automatically adjust bantime/findtime on rare but steady events from one source)
* can use not only logfiles, but anything that can give malicious ip: accept(), recv(), pubsub event, pipe
* may work as honeypot (emulating open tcp ports)

Docs:

* [Installation](docs/install.md) -- generic installation instructions
* [Quickstart](docs/configuration.md) -- config file description and configuration notes

Similar software:

* [fail2ban](http://www.fail2ban.org)
* [sshguard](http://sshguard.sourceforge.net)

License: GPL2+
