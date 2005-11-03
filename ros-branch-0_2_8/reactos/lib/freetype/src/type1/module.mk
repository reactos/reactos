#
# FreeType 2 Type1 module definition
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


make_module_list: add_type1_driver

add_type1_driver:
	$(OPEN_DRIVER)t1_driver_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)type1     $(ECHO_DRIVER_DESC)Postscript font files with extension *.pfa or *.pfb$(ECHO_DRIVER_DONE)

# EOF
