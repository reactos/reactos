

spec2def(console.dll console.spec)

list(APPEND SOURCE
    console.c
    options.c
    font.c
    layout.c
    colors.c
    console.rc
    ${CMAKE_CURRENT_BINARY_DIR}/console.def)

add_library(console SHARED ${SOURCE})

set_module_type(console win32dll UNICODE)

add_importlibs(console
    msvcrt
    user32
    gdi32
    comctl32
    kernel32)

add_pch(console console.h)
add_cd_file(TARGET console DESTINATION reactos/system32 FOR all)
