#
# FreeType 2 host platform detection rules
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# This sub-Makefile is in charge of detecting the current platform.  It sets
# the following variables:
#
#   BUILD        The configuration and system-specific directory.  Usually
#                `freetype/builds/$(PLATFORM)' but can be different for
#                custom builds of the library.
#
# The following variables must be defined in system specific `detect.mk'
# files:
#
#   PLATFORM     The detected platform.  This will default to `ansi' if
#                auto-detection fails.
#   CONFIG_FILE  The configuration sub-makefile to use.  This usually depends
#                on the compiler defined in the `CC' environment variable.
#   DELETE       The shell command used to remove a given file.
#   COPY         The shell command used to copy one file.
#   SEP          The platform-specific directory separator.
#   CC           The compiler to use.
#
# You need to set the following variable(s) before calling it:
#
#   TOP_DIR      The top-most directory in the FreeType library source
#                hierarchy.  If not defined, it will default to `.'.

# If TOP_DIR is not defined, default it to `.'
#
ifndef TOP_DIR
  TOP_DIR := .
endif

# Set auto-detection default to `ansi' resp. UNIX-like operating systems.
# Note that we delay evaluation of $(BUILD_CONFIG_), $(BUILD), and
# $(CONFIG_RULES).
#
PLATFORM := ansi
DELETE   := $(RM)
COPY     := cp
SEP      := /

BUILD_CONFIG_ = $(TOP_DIR)$(SEP)builds$(SEP)
BUILD         = $(BUILD_CONFIG_)$(PLATFORM)
CONFIG_RULES  = $(BUILD)$(SEP)$(CONFIG_FILE)

# We define the BACKSLASH variable to hold a single back-slash character.
# This is needed because a line like
#
#   SEP := \
#
# does not work with GNU Make (the backslash is interpreted as a line
# continuation).  While a line like
#
#   SEP := \\
#
# really defines $(SEP) as `\' on Unix, and `\\' on Dos and Windows!
#
BACKSLASH := $(strip \ )

# Find all auto-detectable platforms.
#
PLATFORMS_ := $(notdir $(subst /detect.mk,,$(wildcard $(BUILD_CONFIG_)*/detect.mk)))
.PHONY: $(PLATFORMS_) ansi

# Filter out platform specified as setup target.
#
PLATFORM := $(firstword $(filter $(MAKECMDGOALS),$(PLATFORMS_)))

# If no setup target platform was specified, enable auto-detection/
# default platform.
#
ifeq ($(PLATFORM),)
  PLATFORM := ansi
endif

# If the user has explicitly asked for `ansi' on the command line,
# disable auto-detection.
#
ifeq ($(findstring ansi,$(MAKECMDGOALS)),)
  # Now, include all detection rule files found in the `builds/<system>'
  # directories.  Note that the calling order of the various `detect.mk'
  # files isn't predictable.
  #
  include $(wildcard $(BUILD_CONFIG_)*/detect.mk)
endif

# In case no detection rule file was successful, use the default.
#
ifndef CONFIG_FILE
  CONFIG_FILE := ansi.mk
  setup: std_setup
  .PHONY: setup
endif

# The following targets are equivalent, with the exception that they use
# a slightly different syntax for the `echo' command.
#
# std_setup: defined for most (i.e. Unix-like) platforms
# dos_setup: defined for Dos-ish platforms like Dos, Windows & OS/2
#
.PHONY: std_setup dos_setup

std_setup:
	@echo ""
	@echo "$(PROJECT_TITLE) build system -- automatic system detection"
	@echo ""
	@echo "The following settings are used:"
	@echo ""
	@echo "  platform                    $(PLATFORM)"
	@echo "  compiler                    $(CC)"
	@echo "  configuration directory     $(BUILD)"
	@echo "  configuration rules         $(CONFIG_RULES)"
	@echo ""
	@echo "If this does not correspond to your system or settings please remove the file"
	@echo "\`$(CONFIG_MK)' from this directory then read the INSTALL file for help."
	@echo ""
	@echo "Otherwise, simply type \`$(MAKE)' again to build the library."
	@echo ""
	@$(COPY) $(CONFIG_RULES) $(CONFIG_MK)


# Special case for Dos, Windows, OS/2, where echo "" doesn't work correctly!
#
dos_setup:
	@type builds\newline
	@echo $(PROJECT_TITLE) build system -- automatic system detection
	@type builds\newline
	@echo The following settings are used:
	@type builds\newline
	@echo   platform