#
# FreeType 2 PSNames driver compilation rules for VMS
#


# Copyright 2001, 2002 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.psnames])

OBJS=psnames.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
