#
# FreeType 2 GZip support configuration rules
#


# Copyright 2002 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# gzip driver directory
#
GZIP_DIR  := $(SRC_)gzip
GZIP_DIR_ := $(GZIP_DIR)$(SEP)


# compilation flags for the driver
#
ifeq ($(SYSTEM_ZLIB),)
  GZIP_COMPILE := $(FT_COMPILE) $I$(GZIP_DIR)
else
  GZIP_COMPILE := $(FT_COMPILE)
endif


# gzip support sources (i.e., C files)
#
GZIP_DRV_SRC := $(GZIP_DIR_)ftgzip.c

# gzip support headers
#
GZIP_DRV_H :=


# gzip driver object(s)
#
#   GZIP_DRV_OBJ_M is used during `multi' builds
#   GZIP_DRV_OBJ_S is used during `single' builds
#
ifeq ($(SYSTEM_ZLIB),)
  GZIP_DRV_OBJ_M := $(GZIP_DRV_SRC:$(GZIP_DIR_)%.c=$(OBJ_)%.$O)
else
  GZIP_DRV_OBJ_M := $(OBJ_)ftgzip.$O
endif
GZIP_DRV_OBJ_S := $(OBJ_)ftgzip.$O

# gzip support source file for single build
#
GZIP_DRV_SRC_S := $(GZIP_DIR_)ftgzip.c


# gzip support - single object
#
$(GZIP_DRV_OBJ_S): $(GZIP_DRV_SRC_S) $(GZIP_DRV_SRC) $(FREETYPE_H) $(GZIP_DRV_H)
	$(GZIP_COMPILE) $T$@ $(GZIP_DRV_SRC_S)


# gzip support - multiple objects
#
$(OBJ_)%.$O: $(GZIP_DIR_)%.c $(FREETYPE_H) $(GZIP_DRV_H)
	$(GZIP_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(GZIP_DRV_OBJ_S)
DRV_OBJS_M += $(GZIP_DRV_OBJ_M)

# EOF
