#
# FreeType 2 auto-hinter module definition
#


# Copyright 2000 Catharon Productions Inc.
# Author: David Turner
#
# This file is part of the Catharon Typography Project and shall only
# be used, modified, and distributed under the terms of the Catharon
# Open Source License that should come with this file under the name
# `CatharonLicense.txt'.  By continuing to use, modify, or distribute
# this file you indicate that you have read the license and
# understand and accept it fully.
#
# Note that this license is compatible with the FreeType license.


make_module_list: add_autohint_module

add_autohint_module:
	$(OPEN_DRIVER)autohint_module_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)autohint  $(ECHO_DRIVER_DESC)automatic hinting module$(ECHO_DRIVER_DONE)

# EOF
