#
# FreeType 2 CID driver configuration rules
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# CID driver directory
#
CID_DIR  := $(SRC_)cid
CID_DIR_ := $(CID_DIR)$(SEP)


CID_COMPILE := $(FT_COMPILE)


# CID driver sources (i.e., C files)
#
CID_DRV_SRC := $(CID_DIR_)cidparse.c \
               $(CID_DIR_)cidload.c  \
               $(CID_DIR_)cidriver.c \
               $(CID_DIR_)cidgload.c \
               $(CID_DIR_)cidobjs.c

# CID driver headers
#
CID_DRV_H := $(CID_DRV_SRC:%.c=%.h) \
             $(CID_DIR_)cidtokens.h


# CID driver object(s)
#
#   CID_DRV_OBJ_M is used during `multi' builds
#   CID_DRV_OBJ_S is used during `single' builds
#
CID_DRV_OBJ_M := $(CID_DRV_SRC:$(CID_DIR_)%.c=$(OBJ_)%.$O)
CID_DRV_OBJ_S := $(OBJ_)type1cid.$O

# CID driver source file for single build
#
CID_DRV_SRC_S := $(CID_DIR_)type1cid.c


# CID driver - single object
#
$(CID_DRV_OBJ_S): $(CID_DRV_SRC_S) $(CID_DRV_SRC) $(FREETYPE_H) $(CID_DRV_H)
	$(CID_COMPILE) $T$@ $(CID_DRV_SRC_S)


# CID driver - multiple objects
#
$(OBJ_)%.$O: $(CID_DIR_)%.c $(FREETYPE_H) $(CID_DRV_H)
	$(CID_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(CID_DRV_OBJ_S)
DRV_OBJS_M += $(CID_DRV_OBJ_M)

# EOF
