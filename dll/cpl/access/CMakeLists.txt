
set_rc_compiler()

spec2def(access.cpl access.spec)

add_library(access SHARED
    access.c
    display.c
    general.c
    keyboard.c
    mouse.c
    sound.c
    access.rc
     ${CMAKE_CURRENT_BINARY_DIR}/access.def)

set_module_type(access cpl UNICODE)

add_importlibs(access
    msvcrt
    user32
    gdi32
    advapi32
    comctl32
    kernel32)

add_pch(access access.h)
add_cd_file(TARGET access DESTINATION reactos/system32 FOR all)
