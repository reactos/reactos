#
# FreeType 2 configuration file to detect a Win32 host platform.
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

  # Detecting Windows NT is easy, as the OS variable must be defined and
  # contains `Windows_NT'.  Untested with Windows 2K, but I guess it should
  # work...
  #
  ifeq ($(OS),Windows_NT)
    is_windows := 1

    # We test for the COMSPEC environment variable, then run the `ver'
    # command-line program to see if its output contains the word `Windows'.
    #
    # If this is true, we are running a win32 platform (or an emulation).
    #
  else
    ifdef COMSPEC
      is_windows := $(findstring Windows,$(strip $(shell ver)))
    endif
  endif  # test NT

  ifdef is_windows

    PLATFORM := win32
    DELETE   := del
    COPY     := copy

    CONFIG_FILE := w32-gcc.mk  # gcc Makefile by default
    SEP         := /
    ifeq ($(firstword $(CC)),cc)
      CC        := gcc
    endif

    # additionally, we provide hooks for various other compilers
    #
    ifneq ($(findstring visualc,$(MAKECMDGOALS)),)     # Visual C/C++
      CONFIG_FILE := w32-vcc.mk
      SEP         := $(BACKSLASH)
      CC          := cl
      visualc: setup
    endif

    ifneq ($(findstring watcom,$(MAKECMDGOALS)),)      # Watcom C/C++
      CONFIG_FILE := w32-wat.mk
      SEP         := $(BACKSLASH)
      CC          := wcc386
      watcom: setup
    endif

    ifneq ($(findstring visualage,$(MAKECMDGOALS)),)   # Visual Age C++
      CONFIG_FILE := w32-icc.mk
      SEP         := $(BACKSLASH)
      CC          := icc
      visualage: setup
    endif

    ifneq ($(findstring lcc,$(MAKECMDGOALS)),)         # LCC-Win32
      CONFIG_FILE := w32-lcc.mk
      SEP         := $(BACKSLASH)
      CC          := lcc
      lcc: setup
    endif

    ifneq ($(findstring devel,$(MAKECMDGOALS)),)       # development target
      CONFIG_FILE := w32-dev.mk
      CC          := gcc
      SEP         := /
      devel: setup
    endif

    setup: dos_setup

  endif # test is_windows
endif   # test PLATFORM

# EOF
