
add_executable(ping ping.c ping.rc)
set_module_type(ping win32cui UNICODE)
add_importlibs(ping ws2_32 iphlpapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ping DESTINATION reactos/system32 FOR all)
