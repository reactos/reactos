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
	    @echo 