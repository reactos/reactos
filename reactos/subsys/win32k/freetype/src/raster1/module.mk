make_module_list: add_raster1_module

add_raster1_module:
	$(OPEN_DRIVER)ft_raster1_renderer_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)raster1   $(ECHO_DRIVER_DESC)monochrome bitmap renderer$(ECHO_DRIVER_DONE)

# EOF
