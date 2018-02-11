add_executable(W32Time W32Time.c ntpclient.c)
set_module_type(W32Time win32cui UNICODE)
add_importlibs(W32Time ws2_32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET W32Time DESTINATION reactos/system32 FOR all)
