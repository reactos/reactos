#
# FreeType 2 Win32 specific definitions
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


DELETE   := del
HOSTSEP  := $(strip \ )
BUILD    := $(TOP_DIR)$(SEP)builds$(SEP)win32
PLATFORM := win32

# by default, we use "\" as a separator on Win32
# but certain compilers accept "/" as well
#
ifndef SEP
  SEP    := $(HOSTSEP)
endif


# The directory where all object files are placed.
#
# This lets you build the library in your own directory with something like
#
#   set TOP_DIR=.../path/to/freetype2/top/dir...
#   set OBJ_DIR=.../path/to/obj/dir
#   make -f %TOP_DIR%/Makefile setup [options]
#   make -f %TOP_DIR%/Makefile
#
ifndef OBJ_DIR
  OBJ_DIR := $(TOP_DIR)$(SEP)objs
endif


# The directory where all library files are placed.
#
# By default, this is the same as $(OBJ_DIR); however, this can be changed
# to suit particular needs.
#
LIB_DIR := $(OBJ_DIR)


# The name of the final library file.  Note that the DOS-specific Makefile
# uses a shorter (8.3) name.
#
LIBRARY := $(PROJECT)


# The NO_OUTPUT macro is used to ignore the output of commands.
#
NO_OUTPUT = 2> nul

# EOF
