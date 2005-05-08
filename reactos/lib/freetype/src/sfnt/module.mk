#
# FreeType 2 SFNT module definition
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


make_module_list: add_sfnt_module

add_sfnt_module:
	$(OPEN_DRIVER)sfnt_module_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)sfnt      $(ECHO_DRIVER_DESC)helper module for TrueType & OpenType formats$(ECHO_DRIVER_DONE)

# EOF
