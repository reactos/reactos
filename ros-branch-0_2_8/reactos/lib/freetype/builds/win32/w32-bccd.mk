#
# FreeType 2 Borland C++ on Win32 + debugging
#


# Copyright 1996-2000, 2003 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


include $(TOP_DIR)/builds/win32/win32-def.mk
BUILD_DIR := $(TOP_DIR)/devel

include $(TOP_DIR)/builds/compiler/bcc-dev.mk

# include linking instructions
include $(TOP_DIR)/builds/link_dos.mk


# EOF
