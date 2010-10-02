
set_unicode()

spec2def(beepmidi ${CMAKE_CURRENT_SOURCE_DIR}/beepmidi.spec ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def)

add_library(beepmidi SHARED beepmidi.c)

set_entrypoint(beepmidi 0)

target_link_libraries(beepmidi ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def)

add_importlibs(beepmidi winmm kernel32 ntdll)
add_dependencies(beepmidi beepmidi_def psdk)
