#
# FreeType 2 auto-hinter module configuration rules
#


# Copyright 2000, 2001, 2002, 2003 Catharon Productions Inc.
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
AUTO_DIR := $(SRC_DIR)/autohint


# compilation flags for the driver
#
AUTO_COMPILE := $(FT_COMPILE) $I$(subst /,$(COMPILER_SEP),$(AUTO_DIR))


# AUTO driver sources (i.e., C files)
#
AUTO_DRV_SRC := $(AUTO_DIR)/ahangles.c \
                $(AUTO_DIR)/ahglobal.c \
                $(AUTO_DIR)/ahglyph.c  \
                $(AUTO_DIR)/ahhint.c   \
                $(AUTO_DIR)/ahmodule.c

# AUTO driver headers
#
AUTO_DRV_H := $(AUTO_DRV_SRC:%c=%h)  \
              $(AUTO_DIR)/ahloader.h \
              $(AUTO_DIR)/ahtypes.h  \
              $(AUTO_DIR)/aherrors.h


# AUTO driver object(s)
#
#   AUTO_DRV_OBJ_M is used during `multi' builds.
#   AUTO_DRV_OBJ_S is used during `single' builds.
#
AUTO_DRV_OBJ_M := $(AUTO_DRV_SRC:$(AUTO_DIR)/%.c=$(OBJ_DIR)/%.$O)
AUTO_DRV_OBJ_S := $(OBJ_DIR)/autohint.$O

# AUTO driver source file for single build
#
AUTO_DRV_SRC_S := $(AUTO_DIR)/autohint.c


# AUTO driver - single object
#
$(AUTO_DRV_OBJ_S): $(AUTO_DRV_SRC_S) $(AUTO_DRV_SRC) \
                   $(FREETYPE_H) $(AUTO_DRV_H)
	$(AUTO_COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $(AUTO_DRV_SRC_S))


# AUTO driver - multiple objects
#
$(OBJ_DIR)/%.$O: $(AUTO_DIR)/%.c $(FREETYPE_H) $(AUTO_DRV_H)
	$(AUTO_COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


# update main driver object lists
#
DRV_OBJS_S += $(AUTO_DRV_OBJ_S)
DRV_OBJS_M += $(AUTO_DRV_OBJ_M)


# EOF
