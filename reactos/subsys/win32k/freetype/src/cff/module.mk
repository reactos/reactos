make_module_list: add_cff_driver

add_cff_driver:
	$(OPEN_DRIVER)cff_driver_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)cff       $(ECHO_DRIVER_DESC)OpenType fonts with extension *.otf$(ECHO_DRIVER_DONE)

# EOF
