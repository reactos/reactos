#
# FreeType 2 configuration rules for mingw32
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# the separator must be set before including win32-def
# as it defaults to "\" on Win32
SEP := /

# include Win32-specific definitions
include $(TOP_DIR)/builds/win32/win32-def.mk

LIBRARY := lib$(PROJECT)

# include gcc-specific definitions
include $(TOP_DIR)/builds/compiler/gcc.mk

# include linking instructions
include $(TOP_DIR)/builds/link_dos.mk

# EOF
