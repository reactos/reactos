make_module_list: add_type1cid_driver

add_type1cid_driver:
	$(OPEN_DRIVER)t1cid_driver_class$(CLOSE_DRIVER)
	$(ECHO_DRIVER)cid       $(ECHO_DRIVER_DESC)Postscript CID-keyed fonts, no known extension$(ECHO_DRIVER_DONE)
# EOF
