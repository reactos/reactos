
set_unicode()

spec2def(input ${CMAKE_CURRENT_SOURCE_DIR}/input.spec ${CMAKE_CURRENT_BINARY_DIR}/input.def)

add_library(input SHARED
    input.c
    settings.c
    keysettings.c
    add.c
    changekeyseq.c
    input.rc)

set_module_type(input win32dll)

target_link_libraries(input ${CMAKE_CURRENT_BINARY_DIR}/input.def)

add_importlibs(input
    msvcrt
    advapi32
    user32
    comctl32
    gdi32
    kernel32)

add_dependencies(input input_def)
add_cab_target(input 1)
