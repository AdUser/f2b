Build process is quite simple:

    cmake .
    make
    make test

Then, if everything builds fine, you may install files in proper places.

    sudo make install

NOTE: This is old good "slackware-way" of installing programs.
You may want to use [checkinstall](http://checkinstall.izto.org) or something like this.

Actual list of configurable options for first line can be found at top of `CMakeLists.txt`.
It looks like this:

    option(SYMBOL "Option description" DEFAULT_VALUE)

Option can be (re)defined with `-DSYMBOL=ON` or `-DSYMBOL=OFF` on cmake command line:

    cmake -D CMAKE_BUILD_TYPE=Release -DWIITH_HARDENING=ON .

Other noticeable options are:

* `CMAKE_BUILD_TYPE` (Debug, Release or unset) -- sets compiler optimization level and debugging info. Set to "Release" for production code.
* `CMAKE_C_COMPILER` -- allows specify another compiler
* `CMAKE_INSTALL_PREFIX` -- set root of install dir ($DESTDIR also will be prefixed if set).
* `INIT_SCRIPT` -- install system init script (values: off/openrc/systemd, default: off)

After building you may type `sudo make install` to install compiled binaries and other files.
Default install layout is:

    /etc/f2b <- configs location
    /etc/f2b/conf-available <- all available config parts
    /etc/f2b/conf-enabled   <- enabled config parts
    /usr/bin  <- non-root binaries
    /usr/sbin <- root binaries
    /usr/lib  <- loadable modules
    /usr/share/f2b <- patterns collection
    /var/lib/f2b <- files with saved states of jails

After install you need additional steps before configuring f2b.

    cd /etc/f2b
    # make symlink to patterns collection
    ln -s /usr/share/f2b/filters filters
    # enable some config parts
    # you need at least one backend, filter and source
    cd /etc/f2b/conf-enabled
    ln -s ../conf-available/05-source-files.conf       ./
    ls -n ../conf-available/10-backend-exec-ipset.conf ./
    ln -s ../conf-available/15-filter-preg.conf        ./
    # make config part for local settings
    touch 99-local.conf

Now you may proceed for configuration and first launch.
