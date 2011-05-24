
add_executable(tftpd tftpd.cpp)

target_link_libraries(tftpd wine)

set_module_type(tftpd win32cui)
add_importlibs(tftpd advapi32 ws2_32 msvcrt kernel32 ntdll)
