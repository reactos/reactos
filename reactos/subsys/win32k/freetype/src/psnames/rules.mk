#
# FreeType 2 PSNames driver configuration rules
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# PSNames driver directory
#
PSNAMES_DIR  := $(SRC_)psnames
PSNAMES_DIR_ := $(PSNAMES_DIR)$(SEP)


# compilation flags for the driver
#
PSNAMES_COMPILE := $(FT_COMPILE)


# PSNames driver sources (i.e., C files)
#
PSNAMES_DRV_SRC := $(PSNAMES_DIR_)psmodule.c


# PSNames driver headers
#
PSNAMES_DRV_H := $(PSNAMES_DRV_SRC:%.c=%.h) \
                 $(PSNAMES_DIR_)pstables.h


# PSNames driver object(s)
#
#   PSNAMES_DRV_OBJ_M is used during `multi' builds
#   PSNAMES_DRV_OBJ_S is used during `single' builds
#
PSNAMES_DRV_OBJ_M := $(PSNAMES_DRV_SRC:$(PSNAMES_DIR_)%.c=$(OBJ_)%.$O)
PSNAMES_DRV_OBJ_S := $(OBJ_)psnames.$O

# PSNames driver source file for single build
#
PSNAMES_DRV_SRC_S := $(PSNAMES_DIR_)psmodule.c


# PSNames driver - single object
#
$(PSNAMES_DRV_OBJ_S): $(PSNAMES_DRV_SRC_S) $(PSNAMES_DRV_SRC) \
                      $(FREETYPE_H) $(PSNAMES_DRV_H)
	$(PSNAMES_COMPILE) $T$@ $(PSNAMES_DRV_SRC_S)


# PSNames driver - multiple objects
#
$(OBJ_)%.$O: $(PSNAMES_DIR_)%.c $(FREETYPE_H) $(PSNAMES_DRV_H)
	$(PSNAMES_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(PSNAMES_DRV_OBJ_S)
DRV_OBJS_M += $(PSNAMES_DRV_OBJ_M)


# EOF
