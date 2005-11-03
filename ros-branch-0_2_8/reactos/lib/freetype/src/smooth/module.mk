#
# FreeType 2 smooth renderer module definition
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


make_module_list: add_smooth_renderer

add_smooth_renderer:
	$(OPEN_DRIVER)ft_smooth_renderer_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)smooth    $(ECHO_DRIVER_DESC)anti-aliased bitmap renderer$(ECHO_DRIVER_DONE)

# EOF
