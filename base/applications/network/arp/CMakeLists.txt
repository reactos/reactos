
add_executable(arp arp.c arp.rc)

set_module_type(arp win32cui)

add_importlibs(arp iphlpapi ws2_32 shlwapi msvcrt kernel32)

add_cab_target(arp 1)