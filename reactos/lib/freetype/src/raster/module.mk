#
# FreeType 2 renderer module definition
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


make_module_list: add_raster_module

add_raster_module:
	$(OPEN_DRIVER)ft_raster1_renderer_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)raster    $(ECHO_DRIVER_DESC)monochrome bitmap renderer$(ECHO_DRIVER_DONE)

# EOF
