#
# FreeType 2 TrueType driver configuration rules
#


# Copyright 1996-2000, 2001 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# TrueType driver directory
#
TT_DIR  := $(SRC_)truetype
TT_DIR_ := $(TT_DIR)$(SEP)


# compilation flags for the driver
#
TT_COMPILE := $(FT_COMPILE) $I$(TT_DIR)


# TrueType driver sources (i.e., C files)
#
TT_DRV_SRC := $(TT_DIR_)ttobjs.c   \
              $(TT_DIR_)ttpload.c  \
              $(TT_DIR_)ttgload.c  \
              $(TT_DIR_)ttinterp.c \
              $(TT_DIR_)ttdriver.c

# TrueType driver headers
#
TT_DRV_H := $(TT_DRV_SRC:%.c=%.h) \
            $(TT_DIR_)tterrors.h


# TrueType driver object(s)
#
#   TT_DRV_OBJ_M is used during `multi' builds
#   TT_DRV_OBJ_S is used during `single' builds
#
TT_DRV_OBJ_M := $(TT_DRV_SRC:$(TT_DIR_)%.c=$(OBJ_)%.$O)
TT_DRV_OBJ_S := $(OBJ_)truetype.$O

# TrueType driver source file for single build
#
TT_DRV_SRC_S := $(TT_DIR_)truetype.c


# TrueType driver - single object
#
$(TT_DRV_OBJ_S): $(TT_DRV_SRC_S) $(TT_DRV_SRC) $(FREETYPE_H) $(TT_DRV_H)
	$(TT_COMPILE) $T$@ $(TT_DRV_SRC_S)


# driver - multiple objects
#
$(OBJ_)%.$O: $(TT_DIR_)%.c $(FREETYPE_H) $(TT_DRV_H)
	$(TT_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(TT_DRV_OBJ_S)
DRV_OBJS_M += $(TT_DRV_OBJ_M)

# EOF
