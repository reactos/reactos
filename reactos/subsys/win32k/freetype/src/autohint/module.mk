make_module_list: add_autohint_module

add_autohint_module:
	$(OPEN_DRIVER)autohint_module_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)autohint  $(ECHO_DRIVER_DESC)automatic hinting module$(ECHO_DRIVER_DONE)

# EOF
