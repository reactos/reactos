#
# FreeType 2 PSaux driver configuration rules
#


# Copyright 1996-2000, 2002 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# PSAUX driver directory
#
PSAUX_DIR  := $(SRC_)psaux
PSAUX_DIR_ := $(PSAUX_DIR)$(SEP)


# compilation flags for the driver
#
PSAUX_COMPILE := $(FT_COMPILE) $I$(PSAUX_DIR)


# PSAUX driver sources (i.e., C files)
#
PSAUX_DRV_SRC := $(PSAUX_DIR_)psobjs.c   \
                 $(PSAUX_DIR_)t1decode.c \
                 $(PSAUX_DIR_)t1cmap.c   \
                 $(PSAUX_DIR_)psauxmod.c

# PSAUX driver headers
#
PSAUX_DRV_H := $(PSAUX_DRV_SRC:%c=%h)  \
               $(PSAUX_DIR_)psauxerr.h


# PSAUX driver object(s)
#
#   PSAUX_DRV_OBJ_M is used during `multi' builds.
#   PSAUX_DRV_OBJ_S is used during `single' builds.
#
PSAUX_DRV_OBJ_M := $(PSAUX_DRV_SRC:$(PSAUX_DIR_)%.c=$(OBJ_)%.$O)
PSAUX_DRV_OBJ_S := $(OBJ_)psaux.$O

# PSAUX driver source file for single build
#
PSAUX_DRV_SRC_S := $(PSAUX_DIR_)psaux.c


# PSAUX driver - single object
#
$(PSAUX_DRV_OBJ_S): $(PSAUX_DRV_SRC_S) $(PSAUX_DRV_SRC) \
                   $(FREETYPE_H) $(PSAUX_DRV_H)
	$(PSAUX_COMPILE) $T$@ $(PSAUX_DRV_SRC_S)


# PSAUX driver - multiple objects
#
$(OBJ_)%.$O: $(PSAUX_DIR_)%.c $(FREETYPE_H) $(PSAUX_DRV_H)
	$(PSAUX_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(PSAUX_DRV_OBJ_S)
DRV_OBJS_M += $(PSAUX_DRV_OBJ_M)


# EOF
