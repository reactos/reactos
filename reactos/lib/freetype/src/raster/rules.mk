#
# FreeType 2 renderer module build rules
#


# Copyright 1996-2000, 2001 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# raster1 driver directory
#
RAS1_DIR  := $(SRC_)raster
RAS1_DIR_ := $(RAS1_DIR)$(SEP)

# compilation flags for the driver
#
RAS1_COMPILE := $(FT_COMPILE) $I$(RAS1_DIR)


# raster1 driver sources (i.e., C files)
#
RAS1_DRV_SRC := $(RAS1_DIR_)ftraster.c \
                $(RAS1_DIR_)ftrend1.c


# raster1 driver headers
#
RAS1_DRV_H := $(RAS1_DRV_SRC:%.c=%.h) \
              $(RAS1_DIR_)rasterrs.h


# raster1 driver object(s)
#
#   RAS1_DRV_OBJ_M is used during `multi' builds.
#   RAS1_DRV_OBJ_S is used during `single' builds.
#
RAS1_DRV_OBJ_M := $(RAS1_DRV_SRC:$(RAS1_DIR_)%.c=$(OBJ_)%.$O)
RAS1_DRV_OBJ_S := $(OBJ_)raster.$O

# raster1 driver source file for single build
#
RAS1_DRV_SRC_S := $(RAS1_DIR_)raster.c


# raster1 driver - single object
#
$(RAS1_DRV_OBJ_S): $(RAS1_DRV_SRC_S) $(RAS1_DRV_SRC) \
                   $(FREETYPE_H) $(RAS1_DRV_H)
	$(RAS1_COMPILE) $T$@ $(RAS1_DRV_SRC_S)


# raster1 driver - multiple objects
#
$(OBJ_)%.$O: $(RAS1_DIR_)%.c $(FREETYPE_H) $(RAS1_DRV_H)
	$(RAS1_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(RAS1_DRV_OBJ_S)
DRV_OBJS_M += $(RAS1_DRV_OBJ_M)


# EOF
