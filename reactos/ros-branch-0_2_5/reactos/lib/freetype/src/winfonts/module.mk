#
# FreeType 2 Windows FNT/FON module definition
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


make_module_list: add_windows_driver

add_windows_driver:
	$(OPEN_DRIVER)winfnt_driver_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)winfnt    $(ECHO_DRIVER_DESC)Windows bitmap fonts with extension *.fnt or *.fon$(ECHO_DRIVER_DONE)

