
add_executable(netstat netstat.c netstat.rc)
set_module_type(netstat win32cui)
add_importlibs(netstat user32 ws2_32 snmpapi iphlpapi msvcrt kernel32)
add_cd_file(TARGET netstat DESTINATION reactos/system32 FOR all)
