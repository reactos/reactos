#
# FreeType 2 Type1z driver configuration rules
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# Type1z driver directory
#
T1Z_DIR  := $(SRC_)type1z
T1Z_DIR_ := $(T1Z_DIR)$(SEP)


# compilation flags for the driver
#
T1Z_COMPILE := $(FT_COMPILE)


# Type1 driver sources (i.e., C files)
#
T1Z_DRV_SRC := $(T1Z_DIR_)z1parse.c  \
               $(T1Z_DIR_)z1load.c   \
               $(T1Z_DIR_)z1driver.c \
               $(T1Z_DIR_)z1afm.c    \
               $(T1Z_DIR_)z1gload.c  \
               $(T1Z_DIR_)z1objs.c

# Type1 driver headers
#
T1Z_DRV_H := $(T1Z_DRV_SRC:%.c=%.h) \
             $(T1Z_DIR_)z1tokens.h


# Type1z driver object(s)
#
#   T1Z_DRV_OBJ_M is used during `multi' builds
#   T1Z_DRV_OBJ_S is used during `single' builds
#
T1Z_DRV_OBJ_M := $(T1Z_DRV_SRC:$(T1Z_DIR_)%.c=$(OBJ_)%.$O)
T1Z_DRV_OBJ_S := $(OBJ_)type1z.$O

# Type1z driver source file for single build
#
T1Z_DRV_SRC_S := $(T1Z_DIR_)type1z.c


# Type1z driver - single object
#
$(T1Z_DRV_OBJ_S): $(T1Z_DRV_SRC_S) $(T1Z_DRV_SRC) $(FREETYPE_H) $(T1Z_DRV_H)
	$(T1Z_COMPILE) $T$@ $(T1Z_DRV_SRC_S)


# Type1z driver - multiple objects
#
$(OBJ_)%.$O: $(T1Z_DIR_)%.c $(FREETYPE_H) $(T1Z_DRV_H)
	$(T1Z_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(T1Z_DRV_OBJ_S)
DRV_OBJS_M += $(T1Z_DRV_OBJ_M)

# EOF
