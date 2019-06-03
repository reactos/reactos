
list(APPEND SOURCE
    netid.c
    netid.rc)

add_library(netid MODULE ${SOURCE})
spec2def(netid netid.dll netid.spec)

set_module_type(netid win32dll UNICODE)
add_importlibs(netid user32 gdi32 comctl32 advapi32 netapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET netid DESTINATION reactos/system32 FOR all)
