#
# FreeType 2 configuration file to detect a UNIX host platform.
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


ifeq ($(PLATFORM),ansi)

  has_init := $(strip $(wildcard /sbin/init))
  ifneq ($(has_init),)

    PLATFORM := unix
    COPY     := cp
    DELETE   := rm -f

    # If a Unix platform is detected, the configure script is called and
    # `unix.mk' is created.
    #
    # Arguments to `configure' should be in the CFG variable.  Example:
    #
    #   make CFG="--prefix=/usr --disable-static"
    #
    # If you need to set CFLAGS or LDFLAGS, do it here also.
    #
    # Feel free to add support for other platform specific compilers in this
    # directory (e.g. solaris.mk + changes here to detect the platform).
    #
    CONFIG_FILE := unix.mk
    setup: unix.mk
    unix: setup

    # If `devel' is the requested target, use `-g -O0' as the default value
    # for CFLAGS if CFLAGS isn't set.
    #
    ifneq ($(findstring devel,$(MAKECMDGOALS)),)
      ifndef CFLAGS
        USE_CFLAGS := CFLAGS="-g -O0"
      endif
      devel: setup
    endif

    setup: std_setup

    unix.mk: builds/unix/unix.in
	    cd builds/unix; $(USE_CFLAGS) ./configure $(CFG)

  endif # test Unix
endif   # test PLATFORM

# EOF
