add_library(wuauserv SHARED
    wuauserv.c
    wuauserv.rc)

set_module_type(wuauserv win32dll UNICODE)
add_importlibs(wuauserv advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wuauserv DESTINATION reactos/system32 FOR all)
