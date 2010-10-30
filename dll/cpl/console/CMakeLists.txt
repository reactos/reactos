
set_unicode()

spec2def(console ${CMAKE_CURRENT_SOURCE_DIR}/console.spec ${CMAKE_CURRENT_BINARY_DIR}/console.def)

list(APPEND SOURCE
    console.c
    options.c
    font.c
    layout.c
    colors.c
    console.rc)

add_library(console SHARED ${CMAKE_CURRENT_BINARY_DIR}/console_console.h.gch ${SOURCE})

set_module_type(console win32dll)

target_link_libraries(console ${CMAKE_CURRENT_BINARY_DIR}/console.def)

add_importlibs(console
    msvcrt
    user32
    gdi32
    comctl32
    kernel32)

add_pch(console ${CMAKE_CURRENT_SOURCE_DIR}/console.h ${SOURCE})
add_dependencies(console console_def)
add_cab_target(console 1)
