#
# FreeType 2 OpenType/CFF driver configuration rules
#


# Copyright 1996-2000, 2001 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# OpenType driver directory
#
CFF_DIR  := $(SRC_)cff
CFF_DIR_ := $(CFF_DIR)$(SEP)


CFF_COMPILE := $(FT_COMPILE) $I$(CFF_DIR)


# CFF driver sources (i.e., C files)
#
CFF_DRV_SRC := $(CFF_DIR_)cffobjs.c   \
               $(CFF_DIR_)cffload.c   \
               $(CFF_DIR_)cffgload.c  \
               $(CFF_DIR_)cffparse.c  \
               $(CFF_DIR_)cffcmap.c   \
               $(CFF_DIR_)cffdrivr.c

# CFF driver headers
#
CFF_DRV_H := $(CFF_DRV_SRC:%.c=%.h) \
             $(CFF_DIR_)cfftoken.h \
             $(CFF_DIR_)cfferrs.h


# CFF driver object(s)
#
#   CFF_DRV_OBJ_M is used during `multi' builds
#   CFF_DRV_OBJ_S is used during `single' builds
#
CFF_DRV_OBJ_M := $(CFF_DRV_SRC:$(CFF_DIR_)%.c=$(OBJ_)%.$O)
CFF_DRV_OBJ_S := $(OBJ_)cff.$O

# CFF driver source file for single build
#
CFF_DRV_SRC_S := $(CFF_DIR_)cff.c


# CFF driver - single object
#
$(CFF_DRV_OBJ_S): $(CFF_DRV_SRC_S) $(CFF_DRV_SRC) $(FREETYPE_H) $(CFF_DRV_H)
	$(CFF_COMPILE) $T$@ $(CFF_DRV_SRC_S)


# CFF driver - multiple objects
#
$(OBJ_)%.$O: $(CFF_DIR_)%.c $(FREETYPE_H) $(CFF_DRV_H)
	$(CFF_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(CFF_DRV_OBJ_S)
DRV_OBJS_M += $(CFF_DRV_OBJ_M)

# EOF
