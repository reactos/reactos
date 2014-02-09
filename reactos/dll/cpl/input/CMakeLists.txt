
spec2def(input.dll input.spec)

list(APPEND SOURCE
    input.c
    settings.c
    advsettings.c
    keysettings.c
    add.c
    changekeyseq.c
    input.h)

add_library(input SHARED
    ${SOURCE}
    input.rc
    ${CMAKE_CURRENT_BINARY_DIR}/input.def)

set_module_type(input win32dll UNICODE)

add_importlibs(input
    msvcrt
    advapi32
    user32
    comctl32
    gdi32
    kernel32)

add_pch(input input.h SOURCE)
add_cd_file(TARGET input DESTINATION reactos/system32 FOR all)
