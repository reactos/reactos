make_module_list: add_sfnt_module

add_sfnt_module:
	$(OPEN_DRIVER)sfnt_module_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)sfnt      $(ECHO_DRIVER_DESC)helper module for TrueType & OpenType formats$(ECHO_DRIVER_DONE)

# EOF
