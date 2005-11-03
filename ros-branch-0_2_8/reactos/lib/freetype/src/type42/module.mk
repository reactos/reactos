#
# FreeType 2 Type42 module definition
#


# Copyright 2002 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


make_module_list: add_type42_driver

add_type42_driver:
	$(OPEN_DRIVER)t42_driver_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)type42     $(ECHO_DRIVER_DESC)Type 42 font files with no known extension$(ECHO_DRIVER_DONE)

# EOF
