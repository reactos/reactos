#
# FreeType 2 configuration file to detect an OS/2 host platform.
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

  ifdef OS2_SHELL

    PLATFORM := os2
    COPY     := copy
    DELETE   := del

    CONFIG_FILE := os2-gcc.mk   # gcc-emx by default
    SEP         := /

    # additionally, we provide hooks for various other compilers
    #
    ifneq ($(findstring visualage,$(MAKECMDGOALS)),)     # Visual Age C++
      CONFIG_FILE := os2-icc.mk
      SEP         := $(BACKSLASH)
      CC          := icc
      .PHONY: visualage
    endif

    ifneq ($(findstring watcom,$(MAKECMDGOALS)),)        # Watcom C/C++
      CONFIG_FILE := os2-wat.mk
      SEP         := $(BACKSLASH)
      CC          := wcc386
      .PHONY: watcom
    endif

    ifneq ($(findstring borlandc,$(MAKECMDGOALS)),)      # Borland C++ 32-bit
      CONFIG_FILE := os2-bcc.mk
      SEP         := $(BACKSLASH)
      CC          := bcc32
      .PHONY: borlandc
    endif

    ifneq ($(findstring devel,$(MAKECMDGOALS)),)         # development target
      CONFIG_FILE := os2-dev.mk
      CC          := gcc
      SEP         := /
      devel: setup
    endif

    setup: dos_setup

  endif # test OS2_SHELL
endif   # test PLATFORM

#EOF
