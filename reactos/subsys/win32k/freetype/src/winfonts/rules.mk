#
# FreeType 2 Windows FNT/FON driver configuration rules
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# Windows driver directory
#
FNT_DIR  := $(SRC_)winfonts
FNT_DIR_ := $(FNT_DIR)$(SEP)


FNT_COMPILE := $(FT_COMPILE)


# Windows driver sources (i.e., C files)
#
FNT_DRV_SRC := $(FNT_DIR_)winfnt.c

# Windows driver headers
#
FNT_DRV_H := $(FNT_DRV_SRC:%.c=%.h)


# Windows driver object(s)
#
#   FNT_DRV_OBJ_M is used during `multi' builds
#   FNT_DRV_OBJ_S is used during `single' builds
#
FNT_DRV_OBJ_M := $(FNT_DRV_SRC:$(FNT_DIR_)%.c=$(OBJ_)%.$O)
FNT_DRV_OBJ_S := $(OBJ_)winfnt.$O

# Windows driver source file for single build
#
FNT_DRV_SRC_S := $(FNT_DIR_)winfnt.c


# Windows driver - single object
#
$(FNT_DRV_OBJ_S): $(FNT_DRV_SRC_S) $(FNT_DRV_SRC) $(FREETYPE_H) $(FNT_DRV_H)
	$(FNT_COMPILE) $T$@ $(FNT_DRV_SRC_S)


# Windows driver - multiple objects
#
$(OBJ_)%.$O: $(FNT_DIR_)%.c $(FREETYPE_H) $(FNT_DRV_H)
	$(FNT_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(FNT_DRV_OBJ_S)
DRV_OBJS_M += $(FNT_DRV_OBJ_M)

# EOF
