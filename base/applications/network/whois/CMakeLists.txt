
add_executable(whois whois.c whois.rc)

set_module_type(whois win32cui)

add_importlibs(whois ws2_32 msvcrt kernel32)

add_livecd_target(whois reactos/system32)