
add_executable(rundll32 rundll32.c rundll32.rc)
set_module_type(rundll32 win32gui UNICODE)
add_importlibs(rundll32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET rundll32 DESTINATION reactos/system32 FOR all)
