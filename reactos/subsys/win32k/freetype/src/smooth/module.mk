make_module_list: add_smooth_renderer

add_smooth_renderer:
	$(OPEN_DRIVER)ft_smooth_renderer_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)smooth    $(ECHO_DRIVER_DESC)anti-aliased bitmap renderer$(ECHO_DRIVER_DONE)

# EOF
