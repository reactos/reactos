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


.PHONY: setup


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

  endif
endif # test PLATFORM ansi

ifeq ($(PLATFORM),win32)

  DELETE   := del
  COPY     := copy

  # gcc Makefile by default
  CONFIG_FILE := w32-gcc.mk
  SEP         := /
  ifeq ($(firstword $(CC)),cc)
    CC        := gcc
  endif

  ifneq ($(findstring list,$(MAKECMDGOALS)),)  # test for the "list" target
    dump_target_list:
	    @echo ÿ
	    @echo $(PROJECT_TITLE) build system -- supported compilers
	    @echo ÿ
	    @echo Several command-line compilers are supported on Win32:
	    @echo ÿ
	    @echo ÿÿmake setupÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿgcc (with Mingw)
	    @echo ÿÿmake setup visualcÿÿÿÿÿÿÿÿÿÿÿÿÿMicrosoft Visual C++
	    @echo ÿÿmake setup bcc32ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿBorland C/C++
	    @echo ÿÿmake setup lccÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿWin32-LCC
	    @echo ÿÿmake setup intelcÿÿÿÿÿÿÿÿÿÿÿÿÿÿIntel C/C++
	    @echo ÿ

    setup: dump_target_list
    .PHONY: dump_target_list list
  else
    setup: dos_setup
  endif

  # additionally, we provide hooks for various other compilers
  #
  ifneq ($(findstring visualc,$(MAKECMDGOALS)),)     # Visual C/C++
    CONFIG_FILE := w32-vcc.mk
    SEP         := $(BACKSLASH)
    CC          := cl
    visualc: setup
    .PHONY: visualc
  endif

  ifneq ($(findstring intelc,$(MAKECMDGOALS)),)      # Intel C/C++
    CONFIG_FILE := w32-intl.mk
    SEP         := $(BACKSLASH)
    CC          := cl
    visualc: setup
    .PHONY: intelc
  endif

  ifneq ($(findstring watcom,$(MAKECMDGOALS)),)      # Watcom C/C++
    CONFIG_FILE := w32-wat.mk
    SEP         := $(BACKSLASH)
    CC          := wcc386
    watcom: setup
    .PHONY: watcom
  endif

  ifneq ($(findstring visualage,$(MAKECMDGOALS)),)   # Visual Age C++
    CONFIG_FILE := w32-icc.mk
    SEP         := $(BACKSLASH)
    CC          := icc
    visualage: setup
    .PHONY: visualage
  endif

  ifneq ($(findstring lcc,$(MAKECMDGOALS)),)         # LCC-Win32
    CONFIG_FILE := w32-lcc.mk
    SEP         := $(BACKSLASH)
    CC          := lcc
    lcc: setup
    .PHONY: lcc
  endif

  ifneq ($(findstring mingw32,$(MAKECMDGOALS)),)     # mingw32
    CONFIG_FILE := w32-mingw32.mk
    SEP         := $(BACKSLASH)
    CC          := gcc
    mingw32: setup
    .PHONY: mingw32
  endif

  ifneq ($(findstring bcc32,$(MAKECMDGOALS)),)       # Borland C++
    CONFIG_FILE := w32-bcc.mk
    SEP         := $(BACKSLASH)
    CC          := bcc32
    bcc32: setup
    .PHONY: bcc32
  endif

  ifneq ($(findstring devel-bcc,$(MAKECMDGOALS)),)   # development target
    CONFIG_FILE := w32-bccd.mk
    CC          := bcc32
    SEP         := /
    devel-bcc: setup
    .PHONY: devel-bcc
  endif

  ifneq ($(findstring devel-gcc,$(MAKECMDGOALS)),)   # development target
    CONFIG_FILE := w32-dev.mk
    CC          := gcc
    SEP         := /
    devel-gcc: setup
    .PHONY: devel-gcc
  endif

endif   # test PLATFORM win32

# EOF
