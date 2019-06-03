
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    ntmarta.c
    ntmarta.rc)

add_library(ntmarta MODULE ${SOURCE})
spec2def(ntmarta ntmarta.dll ntmarta.spec)

set_module_type(ntmarta win32dll UNICODE)
add_importlibs(ntmarta user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ntmarta DESTINATION reactos/system32 FOR all)
