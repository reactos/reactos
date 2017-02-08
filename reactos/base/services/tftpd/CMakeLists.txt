
add_executable(tftpd tftpd.cpp)
set_module_type(tftpd win32cui)
add_importlibs(tftpd advapi32 ws2_32 iphlpapi msvcrt kernel32)
