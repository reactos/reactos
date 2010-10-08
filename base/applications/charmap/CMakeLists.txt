
set_unicode()

list(APPEND SOURCE
    about.c
    charmap.c
    lrgcell.c
    map.c
    charmap.rc)

add_executable(charmap
    ${CMAKE_CURRENT_BINARY_DIR}/charmap_precomp.h.gch
    ${SOURCE})

add_pch(charmap ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(charmap win32gui)

add_importlibs(charmap user32 gdi32 comctl32 kernel32 msvcrt)
