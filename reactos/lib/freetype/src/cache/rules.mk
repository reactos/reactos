#
# FreeType 2 Cache configuration rules
#


# Copyright 2000, 2001 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# Cache driver directory
#
CACHE_DIR  := $(SRC_)cache
CACHE_DIR_ := $(CACHE_DIR)$(SEP)

CACHE_H_DIR  := $(PUBLIC_)cache
CACHE_H_DIR_ := $(CACHE_H_DIR)$(SEP)

# compilation flags for the driver
#
Cache_COMPILE := $(FT_COMPILE) $I$(CACHE_DIR)


# Cache driver sources (i.e., C files)
#
Cache_DRV_SRC := $(CACHE_DIR_)ftlru.c    \
                 $(CACHE_DIR_)ftcmanag.c \
                 $(CACHE_DIR_)ftccache.c \
                 $(CACHE_DIR_)ftcglyph.c \
                 $(CACHE_DIR_)ftcsbits.c \
                 $(CACHE_DIR_)ftcimage.c \
                 $(CACHE_DIR_)ftccmap.c

# Cache driver headers
#
Cache_DRV_H := $(CACHE_H_DIR_)ftlru.h    \
               $(CACHE_H_DIR_)ftcmanag.h \
               $(CACHE_H_DIR_)ftcglyph.h \
               $(CACHE_H_DIR_)ftcimage.h \
               $(CACHE_DIR_)ftcerror.h


# Cache driver object(s)
#
#   Cache_DRV_OBJ_M is used during `multi' builds.
#   Cache_DRV_OBJ_S is used during `single' builds.
#
Cache_DRV_OBJ_M := $(Cache_DRV_SRC:$(CACHE_DIR_)%.c=$(OBJ_)%.$O)
Cache_DRV_OBJ_S := $(OBJ_)ftcache.$O

# Cache driver source file for single build
#
Cache_DRV_SRC_S := $(CACHE_DIR_)ftcache.c


# Cache driver - single object
#
$(Cache_DRV_OBJ_S): $(Cache_DRV_SRC_S) $(Cache_DRV_SRC) \
                   $(FREETYPE_H) $(Cache_DRV_H)
	$(Cache_COMPILE) $T$@ $(Cache_DRV_SRC_S)


# Cache driver - multiple objects
#
$(OBJ_)%.$O: $(CACHE_DIR_)%.c $(FREETYPE_H) $(Cache_DRV_H)
	$(Cache_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(Cache_DRV_OBJ_S)
DRV_OBJS_M += $(Cache_DRV_OBJ_M)


# EOF
