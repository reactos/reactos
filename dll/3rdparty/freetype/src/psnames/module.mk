#
# FreeType 2 PSnames module definition
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


make_module_list: add_psnames_module

add_psnames_module:
	$(OPEN_DRIVER)psnames_module_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)psnames   $(ECHO_DRIVER_DESC)Postscript & Unicode Glyph name handling$(ECHO_DRIVER_DONE)

# EOF
