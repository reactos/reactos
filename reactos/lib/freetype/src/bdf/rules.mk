#
# FreeType 2 bdf driver configuration rules
#


# Copyright (C) 2001, 2002 by
# Francesco Zappa Nardelli
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.




# bdf driver directory
#
BDF_DIR  := $(SRC_)bdf
BDF_DIR_ := $(BDF_DIR)$(SEP)


BDF_COMPILE := $(FT_COMPILE) $I$(BDF_DIR)


# bdf driver sources (i.e., C files)
#
BDF_DRV_SRC := $(BDF_DIR_)bdflib.c $(BDF_DIR_)bdfdrivr.c


# bdf driver headers
#
BDF_DRV_H := $(BDF_DIR_)bdf.h \
             $(BDF_DIR_)bdfdrivr.h

# bdf driver object(s)
#
#   BDF_DRV_OBJ_M is used during `multi' builds
#   BDF_DRV_OBJ_S is used during `single' builds
#
BDF_DRV_OBJ_M := $(BDF_DRV_SRC:$(BDF_DIR_)%.c=$(OBJ_)%.$O)
BDF_DRV_OBJ_S := $(OBJ_)bdf.$O

# bdf driver source file for single build
#
BDF_DRV_SRC_S := $(BDF_DIR_)bdf.c


# bdf driver - single object
#
$(BDF_DRV_OBJ_S): $(BDF_DRV_SRC_S) $(BDF_DRV_SRC) $(FREETYPE_H) $(BDF_DRV_H)
	$(BDF_COMPILE) $T$@ $(BDF_DRV_SRC_S)


# bdf driver - multiple objects
#
$(OBJ_)%.$O: $(BDF_DIR_)%.c $(FREETYPE_H) $(BDF_DRV_H)
	$(BDF_COMPILE) $T$@ $<


# update main driver object lists
#
DRV_OBJS_S += $(BDF_DRV_OBJ_S)
DRV_OBJS_M += $(BDF_DRV_OBJ_M)

# EOF
