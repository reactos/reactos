#
# FreeType 2 base layer configuration rules
#


# Copyright 1996-2000, 2002 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# It sets the following variables which are used by the master Makefile
# after the call:
#
#   BASE_OBJ_S:   The single-object base layer.
#   BASE_OBJ_M:   A list of all objects for a multiple-objects build.
#   BASE_EXT_OBJ: A list of base layer extensions, i.e., components found
#                 in `freetype/src/base' which are not compiled within the
#                 base layer proper.
#
# BASE_H is defined in freetype.mk to simplify the dependency rules.


BASE_COMPILE := $(FT_COMPILE) $I$(SRC_)base


# Base layer sources
#
#   ftsystem, ftinit, and ftdebug are handled by freetype.mk
#
BASE_SRC := $(BASE_)ftcalc.c   \
            $(BASE_)fttrigon.c \
            $(BASE_)ftutil.c   \
            $(BASE_)ftstream.c \
            $(BASE_)ftgloadr.c \
            $(BASE_)ftoutln.c  \
            $(BASE_)ftobjs.c   \
            $(BASE_)ftapi.c    \
            $(BASE_)ftnames.c  \
            $(BASE_)ftdbgmem.c

# Base layer `extensions' sources
#
# An extension is added to the library file (.a or .lib) as a separate
# object.  It will then be linked to the final executable only if one of its
# symbols is used by the application.
#
BASE_EXT_SRC := $(BASE_)ftglyph.c \
                $(BASE_)ftmm.c    \
                $(BASE_)ftbdf.c   \
                $(BASE_)fttype1.c \
                $(BASE_)ftxf86.c  \
                $(BASE_)ftpfr.c   \
                $(BASE_)ftbbox.c

# Default extensions objects
#
BASE_EXT_OBJ := $(BASE_EXT_SRC:$(BASE_)%.c=$(OBJ_)%.$O)


# Base layer object(s)
#
#   BASE_OBJ_M is used during `multi' builds (each base source file compiles
#   to a single object file).
#
#   BASE_OBJ_S is used during `single' builds (the whole base layer is
#   compiled as a single object file using ftbase.c).
#
BASE_OBJ_M := $(BASE_SRC:$(BASE_)%.c=$(OBJ_)%.$O)
BASE_OBJ_S := $(OBJ_)ftbase.$O

# Base layer root source file for single build
#
BASE_SRC_S := $(BASE_)ftbase.c


# Base layer - single object build
#
$(BASE_OBJ_S): $(BASE_SRC_S) $(BASE_SRC) $(FREETYPE_H)
	$(BASE_COMPILE) $T$@ $(BASE_SRC_S)


# Multiple objects build + extensions
#
$(OBJ_)%.$O: $(BASE_)%.c $(FREETYPE_H)
	$(BASE_COMPILE) $T$@ $<

# EOF
