make_module_list: add_windows_driver

add_windows_driver:
	$(OPEN_DRIVER)winfnt_driver_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)winfnt    $(ECHO_DRIVER_DESC)Windows bitmap fonts with extension *.fnt or *.fon$(ECHO_DRIVER_DONE)

