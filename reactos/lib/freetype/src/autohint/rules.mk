#
# FreeType 2 auto-hinter module configuration rules
#


# Copyright 2000, 2001 Catharon Productions Inc.
# Author: David Turner
#
# This file is part of the Catharon Typography Project and shall only
# be used, modified, and distributed under the terms of the Catharon
# Open Source License that should come with this file under the name
# `CatharonLicense.txt'.  By continuing to use, modify, or distribute
# this file you indicate that you have read the license and
# understand and accept it fully.
#
# Note that this license is compatible with the FreeType license.


# AUTO driver directory
#
AUTO_DIR  := $(SRC_)autohint
AUTO_DIR_ := $(AUTO_DIR)$(SEP)


# compilation flags for the driver
#
AUTO_COMPILE := $(FT_COMPILE) $I$(AUTO_DIR)


# AUTO driver sources (i.e., C files)
#
AUTO_DRV_SRC := $(AUTO_DIR_)ahangles.c  \
                $(AUTO_DIR_)ahglobal.c  \
                $(AUTO_DIR_)ahglyph.c   \
                $(AUTO_DIR_)ahhint.c    \
                $(AUTO_DIR_)ahmodule.c

# AUTO driver headers
#
AUTO_DRV_H := $(AUTO_DRV_SRC:%c=%h)  \
              $(AUTO_DIR_)ahloader.h \
              $(AUTO_DIR_)ahtypes.h \
              $(AUTO_DIR_)aherrors.h


# AUTO driver object(s)
#
#   AUTO_DRV_OBJ_M is used during `multi' builds.
#   AUTO_DRV_OBJ_S is used during `single' builds.
#
AUTO_DRV_OBJ_M := $(AUTO_DRV_SRC:$(AUTO_DIR_)%.c=$(OBJ_)%.$O)
AUTO_DRV_OBJ_S := $(OBJ_)autohint.$O

# AUTO driver source file for single build
#
AUTO_DRV_SRC_S := $(AUTO_DIR_)autohint.c


# AUTO driver - single object
#
$(AUTO_DRV_OBJ_S): $(AUTO_DRV_SRC_S) $(AUTO_DRV_SRC) \
                   $(FREETYPE_H) $(AUTO_DRV_H)
	$(AUTO_COMPILE) $T$@ $(AUTO_DRV_SRC_S)


# AUTO driver - multiple objects
#
$(OBJ_)%.$O: $(AUTO_DIR_)%.c $(FREETYPE_H) $(AUTO_DRV_H)
	$(AUTO_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(AUTO_DRV_OBJ_S)
DRV_OBJS_M += $(AUTO_DRV_OBJ_M)


# EOF
