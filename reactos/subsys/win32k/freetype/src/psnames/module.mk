make_module_list: add_psnames_module

add_psnames_module:
	$(OPEN_DRIVER)psnames_module_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)psnames   $(ECHO_DRIVER_DESC)Postscript & Unicode Glyph name handling$(ECHO_DRIVER_DONE)

# EOF
