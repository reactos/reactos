#
# FreeType 2 SFNT driver configuration rules
#


# Copyright 1996-2000, 2002 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# SFNT driver directory
#
SFNT_DIR  := $(SRC_)sfnt
SFNT_DIR_ := $(SFNT_DIR)$(SEP)


# compilation flags for the driver
#
SFNT_COMPILE := $(FT_COMPILE) $I$(SFNT_DIR)


# SFNT driver sources (i.e., C files)
#
SFNT_DRV_SRC := $(SFNT_DIR_)ttload.c   \
                $(SFNT_DIR_)ttcmap.c   \
                $(SFNT_DIR_)ttcmap0.c  \
                $(SFNT_DIR_)ttsbit.c   \
                $(SFNT_DIR_)ttpost.c   \
                $(SFNT_DIR_)sfobjs.c   \
                $(SFNT_DIR_)sfdriver.c

# SFNT driver headers
#
SFNT_DRV_H := $(SFNT_DRV_SRC:%c=%h) \
              $(SFNT_DIR_)sferrors.h


# SFNT driver object(s)
#
#   SFNT_DRV_OBJ_M is used during `multi' builds.
#   SFNT_DRV_OBJ_S is used during `single' builds.
#
SFNT_DRV_OBJ_M := $(SFNT_DRV_SRC:$(SFNT_DIR_)%.c=$(OBJ_)%.$O)
SFNT_DRV_OBJ_S := $(OBJ_)sfnt.$O

# SFNT driver source file for single build
#
SFNT_DRV_SRC_S := $(SFNT_DIR_)sfnt.c


# SFNT driver - single object
#
$(SFNT_DRV_OBJ_S): $(SFNT_DRV_SRC_S) $(SFNT_DRV_SRC) \
                   $(FREETYPE_H) $(SFNT_DRV_H)
	$(SFNT_COMPILE) $T$@ $(SFNT_DRV_SRC_S)


# SFNT driver - multiple objects
#
$(OBJ_)%.$O: $(SFNT_DIR_)%.c $(FREETYPE_H) $(SFNT_DRV_H)
	$(SFNT_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(SFNT_DRV_OBJ_S)
DRV_OBJS_M += $(SFNT_DRV_OBJ_M)


# EOF
