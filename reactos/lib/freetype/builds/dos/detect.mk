#
# FreeType 2 configuration file to detect a DOS host platform.
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


.PHONY: setup


ifeq ($(PLATFORM),ansi)

  # Test for DJGPP by checking the DJGPP environment variable, which must be
  # set in order to use the system (ie. it will always be present when the
  # `make' utility is run).
  #
  # We test for the COMSPEC environment variable, then run the `ver'
  # command-line program to see if its output contains the word `Dos'.
  #
  # If this is true, we are running a Dos-ish platform (or an emulation).
  #
  ifdef DJGPP
    PLATFORM := dos
  else
    ifdef COMSPEC
      is_dos := $(findstring Dos,$(shell ver))

      # We try to recognize a Dos session under OS/2.  The `ver' command
      # returns `Operating System/2 ...' there, so `is_dos' should be empty.
      #
      # To recognize a Dos session under OS/2, we check COMSPEC for the
      # substring `MDOS\COMMAND'
      #
      ifeq ($(is_dos),)
        is_dos := $(findstring MDOS\COMMAND,$(COMSPEC))
      endif
    endif # test COMSPEC

    ifneq ($(is_dos),)

      PLATFORM := dos

    endif # test Dos
  endif # test DJGPP
endif # test PLATFORM ansi

ifeq ($(PLATFORM),dos)

  # Use DJGPP (i.e. gcc) by default.
  #
  CONFIG_FILE := dos-gcc.mk
  SEP         := /
  ifndef CC
    CC        := gcc
  endif

  # additionally, we provide hooks for various other compilers
  #
  ifneq ($(findstring turboc,$(MAKECMDGOALS)),)     # Turbo C
    CONFIG_FILE := dos-tcc.mk
    SEP         := $(BACKSLASH)
    CC          := tcc
    turboc: setup
    .PHONY: turboc
  endif

  ifneq ($(findstring watcom,$(MAKECMDGOALS)),)     # Watcom C/C++
    CONFIG_FILE := dos-wat.mk
    SEP         := $(BACKSLASH)
    CC          := wcc386
    watcom: setup
    .PHONY: watcom
  endif

  ifneq ($(findstring borlandc,$(MAKECMDGOALS)),)   # Borland C/C++ 32-bit
    CONFIG_FILE := dos-bcc.mk
    SEP         := $(BACKSLASH)
    CC          := bcc32
    borlandc: setup
    .PHONY: borlandc
  endif

  ifneq ($(findstring borlandc16,$(MAKECMDGOALS)),) # Borland C/C++ 16-bit
    CONFIG_FILE := dos-bcc.mk
    SEP         := $(BACKSLASH)
    CC          := bcc
    borlandc16: setup
    .PHONY: borlandc16
  endif

  ifneq ($(findstring bash,$(SHELL)),)              # check for bash
    DELETE := rm
    COPY   := cp
    setup: std_setup
  else
    DELETE := del
    COPY   := copy
    setup: dos_setup
  endif

endif     # test PLATFORM dos

# EOF
