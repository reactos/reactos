#
# FreeType 2 Type42 driver configuration rules
#


# Copyright 2002 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# Type42 driver directory
#
T42_DIR  := $(SRC_)type42
T42_DIR_ := $(T42_DIR)$(SEP)


# compilation flags for the driver
#
T42_COMPILE := $(FT_COMPILE) $I$(T42_DIR)


# Type42 driver source
#
T42_DRV_SRC := $(T42_DIR_)t42objs.c  \
               $(T42_DIR_)t42parse.c \
               $(T42_DIR_)t42drivr.c

# Type42 driver headers
#
T42_DRV_H := $(T42_DRV_SRC:%.c=%.h) \
             $(T42_DIR_)t42error.h


# Type42 driver object(s)
#
#   T42_DRV_OBJ_M is used during `multi' builds
#   T42_DRV_OBJ_S is used during `single' builds
#
T42_DRV_OBJ_M := $(T42_DRV_SRC:$(T42_DIR_)%.c=$(OBJ_)%.$O)
T42_DRV_OBJ_S := $(OBJ_)type42.$O

# Type42 driver source file for single build
#
T42_DRV_SRC_S := $(T42_DIR_)type42.c


# Type42 driver - single object
#
$(T42_DRV_OBJ_S): $(T42_DRV_SRC_S) $(T42_DRV_SRC) $(FREETYPE_H) $(T42_DRV_H)
	$(T42_COMPILE) $T$@ $(T42_DRV_SRC_S)


# Type42 driver - multiple objects
#
$(OBJ_)%.$O: $(T42_DIR_)%.c $(FREETYPE_H) $(T42_DRV_H)
	$(T42_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(T42_DRV_OBJ_S)
DRV_OBJS_M += $(T42_DRV_OBJ_M)

# EOF
