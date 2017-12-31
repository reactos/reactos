
add_executable(tracert tracert.cpp tracert.rc)
set_module_type(tracert win32cui UNICODE)
add_importlibs(tracert ws2_32 iphlpapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET tracert DESTINATION reactos/system32 FOR all)
