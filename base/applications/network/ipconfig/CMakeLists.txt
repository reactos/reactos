
add_executable(ipconfig ipconfig.c ipconfig.rc)
set_module_type(ipconfig win32cui)
add_importlibs(ipconfig user32 iphlpapi dnsapi advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ipconfig DESTINATION reactos/system32 FOR all)
