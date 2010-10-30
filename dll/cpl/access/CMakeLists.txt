
set_unicode()

spec2def(access ${CMAKE_CURRENT_SOURCE_DIR}/access.spec ${CMAKE_CURRENT_BINARY_DIR}/access.def)

add_library(access SHARED
    access.c
    display.c
    general.c
    keyboard.c
    mouse.c
    sound.c
    access.rc)

set_module_type(access cpl)

target_link_libraries(access ${CMAKE_CURRENT_BINARY_DIR}/access.def)

add_importlibs(access
    msvcrt
    user32
    gdi32
    advapi32
    comctl32
    kernel32)

add_dependencies(access access_def)
add_cab_target(access 1)
