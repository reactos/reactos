
set_unicode()

spec2def(wing32.dll wing32.spec)

add_library(wing32 SHARED wing32.c
    ${CMAKE_CURRENT_BINARY_DIR}/wing32.def)

set_entrypoint(wing32 0)

add_importlibs(wing32 user32 gdi32)
add_dependencies(wing32 psdk)
