#
# FreeType 2 modules sub-Makefile
#

# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# DO NOT INVOKE THIS MAKEFILE DIRECTLY!  IT IS MEANT TO BE INCLUDED BY
# OTHER MAKEFILES.


# This file is in charge of handling the generation of the modules list
# file.

.PHONY: make_module_list clean_module_list remake_module_list

# MODULE_LIST, as its name suggests, indicates where the modules list
# resides.  For now, it is in `include/freetype/config/ftmodule.h'.
#
ifndef FT_MODULE_LIST
  FT_MODULE_LIST := $(TOP)$(SEP)include$(SEP)freetype$(SEP)config$(SEP)ftmodule.h
endif

# To build the modules list, we invoke the `make_module_list' target.
#
# This rule is commented out by default since FreeType comes already with
# a ftmodule.h file.
#
#$(FT_MODULE_LIST): make_module_list

# Before the modules list file can be generated, we must remove the file in
# order to `clean' the list.
#
clean_module_list:
	@-$(DELETE) $(subst $(SEP),$(HOSTSEP),$(FT_MODULE_LIST))
	@-echo Regenerating the modules list in $(FT_MODULE_LIST)...

make_module_list: clean_module_list
	@echo done.


# Trailing spaces are protected with a `#' sign to avoid accidental
# removing.
#
ifneq ($(findstring $(PLATFORM),dos win32 win16 os2),)
  OPEN_MODULE  := @echo #
  CLOSE_MODULE :=  >> $(subst $(SEP),$(HOSTSEP),$(FT_MODULE_LIST))
else
  OPEN_MODULE  := @echo "
  CLOSE_MODULE := " >> $(FT_MODULE_LIST)
endif

# $(OPEN_DRIVER) & $(CLOSE_DRIVER) are used to specify a given font driver
# in the `module.mk' rules file.
#
OPEN_DRIVER  := $(OPEN_MODULE)FT_USE_MODULE(
CLOSE_DRIVER := )$(CLOSE_MODULE)

ECHO_DRIVER      := @echo "* module: #
ECHO_DRIVER_DESC := (
ECHO_DRIVER_DONE := )"

# Each `module.mk' in the `src' sub-dirs is used to add one rule to the
# target `make_module_list'.
#
include $(wildcard $(TOP)/src/*/module.mk)

# EOF
