
add_message_headers(UNICODE arp_msg.mc)
add_executable(arp arp.c arp.rc)
set_module_type(arp win32cui)
add_dependencies(arp arp_msg)
add_importlibs(arp iphlpapi ws2_32 shlwapi msvcrt kernel32)
add_cd_file(TARGET arp DESTINATION reactos/system32 FOR all)
