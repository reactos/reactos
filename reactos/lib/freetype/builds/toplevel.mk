#
# FreeType build system -- top-level sub-Makefile
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# This file is designed for GNU Make, do not use it with another Make tool!
#
# It works as follows:
#
# - When invoked for the first time, this Makefile will include the rules
#   found in `PROJECT/builds/detect.mk'.  They are in charge of detecting
#   the current platform.
#
#   A summary of the detection will be displayed, and the file `config.mk'
#   will be created in the current directory.
#
# - When invoked later, this Makefile will include the rules found in
#   `config.mk'.  This sub-Makefile will define some system-specific
#   variables (like compiler, compilation flags, object suffix, etc.), then
#   include the rules found in `PROJECT/builds/PROJECT.mk', used to build
#   the library.
#
# See the comments in `builds/detect.mk' and `builds/PROJECT.mk' for more
# details on host platform detection and library builds.


.PHONY: all setup distclean modules

# The `space' variable is used to avoid trailing spaces in defining the
# `T' variable later.
#
empty :=
space := $(empty) $(empty)


ifndef CONFIG_MK
  CONFIG_MK := config.mk
endif

# If no configuration sub-makefile is present, or if `setup' is the target
# to be built, run the auto-detection rules to figure out which
# configuration rules file to use.
#
# Note that the configuration file is put in the current directory, which is
# not necessarily $(TOP_DIR).

# If `config.mk' is not present, set `check_platform'.
#
ifeq ($(wildcard $(CONFIG_MK)),)
  check_platform := 1
endif

# If `setup' is one of the targets requested, set `check_platform'.
#
ifneq ($(findstring setup,$(MAKECMDGOALS)),)
  check_platform := 1
endif

# Include the automatic host platform detection rules when we need to
# check the platform.
#
ifdef check_platform

  # This is the first rule `make' sees.
  #
  all: setup

  ifdef USE_MODULES
    # If the module list $(MODULE_LIST) file is not present, generate it.
    #
    #modules: make_module_list setup
  endif

  include $(TOP_DIR)/builds/detect.mk

  ifdef USE_MODULES
    include $(TOP_DIR)/builds/modules.mk

    ifeq ($(wildcard $(MODULE_LIST)),)
      setup: make_module_list
    endif
  endif

  # This rule makes sense for Unix only to remove files created by a run
  # of the configure script which hasn't been successful (so that no
  # `config.mk' has been created).  It uses the built-in $(RM) command of
  # GNU make.  Similarly, `nul' is created if e.g. `make setup win32' has
  # been erroneously used.
  #
  # note: This test is duplicated in "builds/toplevel.mk".
  #
  is_unix := $(strip $(wildcard /sbin/init) $(wildcard /usr/sbin/init) $(wildcard /hurd/auth))
  ifneq ($(is_unix),)

    distclean:
	  $(RM) builds/unix/config.cache
	  $(RM) builds/unix/config.log
	  $(RM) builds/unix/config.status
	  $(RM) builds/unix/unix-def.mk
	  $(RM) builds/unix/unix-cc.mk
	  $(RM) nul

  endif # test is_unix

  # IMPORTANT:
  #
  # `setup' must be defined by the host platform detection rules to create
  # the `config.mk' file in the current directory.

else

  # A configuration sub-Makefile is present -- simply run it.
  #
  all: single

  ifdef USE_MODULES
    modules: make_module_list
  endif

  BUILD_PROJECT := yes
  include $(CONFIG_MK)

endif # test check_platform

# EOF
