#
# FreeType 2 auto-hinter module compilation rules for VMS
#


# Copyright 2001, 2002 Catharon Productions Inc.
#
# This file is part of the Catharon Typography Project and shall only
# be used, modified, and distributed under the terms of the Catharon
# Open Source License that should come with this file under the name
# `CatharonLicense.txt'.  By continuing to use, modify, or distribute
# this file you indicate that you have read the license and
# understand and accept it fully.
#
# Note that this license is compatible with the FreeType license.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/incl=([--.include],[--.src.autohint])

OBJS=autohint.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
