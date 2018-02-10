add_executable(W32Time W32Time.c)
set_module_type(W32Time win32cui UNICODE)
add_importlibs(W32Time advapi32 msvcrt kernel32 ntdll ws2_32)
add_cd_file(TARGET W32Time DESTINATION reactos/system32 FOR all)
