
add_executable(telnetd
    telnetd.c
    serviceentry.c
    telnetd.rc)

target_link_libraries(telnetd wine)
set_module_type(telnetd win32cui)
add_importlibs(telnetd advapi32 ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET telnetd DESTINATION reactos/system32 FOR all)
