
spec2def(access.cpl access.spec)

list(APPEND SOURCE
    access.c
    display.c
    general.c
    keyboard.c
    mouse.c
    sound.c
    access.h)

add_library(access SHARED
    ${SOURCE}
    access.rc
    ${CMAKE_CURRENT_BINARY_DIR}/access.def)

set_module_type(access cpl UNICODE)
add_importlibs(access user32 gdi32 advapi32 comctl32 msvcrt kernel32)
add_pch(access access.h SOURCE)
add_cd_file(TARGET access DESTINATION reactos/system32 FOR all)
