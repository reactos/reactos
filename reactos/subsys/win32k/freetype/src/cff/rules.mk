#
# FreeType 2 OpenType/CFF driver configuration rules
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# OpenType driver directory
#
T2_DIR  := $(SRC_)cff
T2_DIR_ := $(T2_DIR)$(SEP)


T2_COMPILE := $(FT_COMPILE)


# T2 driver sources (i.e., C files)
#
T2_DRV_SRC := $(T2_DIR_)t2objs.c   \
              $(T2_DIR_)t2load.c   \
              $(T2_DIR_)t2gload.c  \
              $(T2_DIR_)t2parse.c  \
              $(T2_DIR_)t2driver.c

# T2 driver headers
#
T2_DRV_H := $(T2_DRV_SRC:%.c=%.h) \
            $(T2_DIR_)t2tokens.h


# T2 driver object(s)
#
#   T2_DRV_OBJ_M is used during `multi' builds
#   T2_DRV_OBJ_S is used during `single' builds
#
T2_DRV_OBJ_M := $(T2_DRV_SRC:$(T2_DIR_)%.c=$(OBJ_)%.$O)
T2_DRV_OBJ_S := $(OBJ_)cff.$O

# T2 driver source file for single build
#
T2_DRV_SRC_S := $(T2_DIR_)cff.c


# T2 driver - single object
#
$(T2_DRV_OBJ_S): $(T2_DRV_SRC_S) $(T2_DRV_SRC) $(FREETYPE_H) $(T2_DRV_H)
	$(T2_COMPILE) $T$@ $(T2_DRV_SRC_S)


# T2 driver - multiple objects
#
$(OBJ_)%.$O: $(T2_DIR_)%.c $(FREETYPE_H) $(T2_DRV_H)
	$(T2_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(T2_DRV_OBJ_S)
DRV_OBJS_M += $(T2_DRV_OBJ_M)

# EOF
