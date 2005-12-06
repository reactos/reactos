#
# FreeType 2 auto-fitter module definition
#


# Copyright 2003, 2004, 2005 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


make_module_list: add_autofit_module

add_autofit_module:
	$(OPEN_DRIVER)autofit_module_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)autofit  $(ECHO_DRIVER_DESC)automatic hinting module$(ECHO_DRIVER_DONE)

# EOF
