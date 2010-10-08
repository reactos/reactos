
set_unicode()

spec2def(wing32 ${CMAKE_CURRENT_SOURCE_DIR}/wing32.spec ${CMAKE_CURRENT_BINARY_DIR}/wing32.def)

add_library(wing32 SHARED wing32.c)

set_entrypoint(wing32 0)

target_link_libraries(wing32 ${CMAKE_CURRENT_BINARY_DIR}/wing32.def)

add_importlibs(wing32 user32 gdi32)
add_dependencies(wing32 wing32_def psdk)
