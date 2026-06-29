
add_executable(ipconfig ipconfig.c ipconfig.rc)
set_module_type(ipconfig win32cui UNICODE)
target_link_libraries(ipconfig conutils)
add_importlibs(ipconfig iphlpapi dnsapi dhcpcsvc advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ipconfig DESTINATION reactos/system32 FOR all)
