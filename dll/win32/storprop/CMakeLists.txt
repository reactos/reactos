
spec2def(storprop.dll storprop.spec)

list(APPEND SOURCE
    disk.c
    dvd.c
    ide.c
    storprop.c)

add_library(storprop MODULE
    ${SOURCE}
    storprop.rc
    ${CMAKE_CURRENT_BINARY_DIR}/storprop.def)

set_module_type(storprop win32dll UNICODE)
add_importlibs(storprop setupapi comctl32 advapi32 user32 msvcrt kernel32 ntdll)
add_pch(storprop precomp.h SOURCE)
add_cd_file(TARGET storprop DESTINATION reactos/system32 FOR all)
