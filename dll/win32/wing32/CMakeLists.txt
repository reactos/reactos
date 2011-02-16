
set_unicode()

spec2def(wing32.dll wing32.spec)

list(APPEND SOURCE
    wing32.c
    ${CMAKE_CURRENT_BINARY_DIR}/wing32.def)

add_library(wing32 SHARED ${SOURCE})

set_entrypoint(wing32 0)

add_importlibs(wing32 user32 gdi32 msvcrt)
add_dependencies(wing32 psdk)
add_cab_target(wing32 1)
