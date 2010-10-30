
set_unicode()

spec2def(beepmidi.dll beepmidi.spec)

add_library(beepmidi SHARED beepmidi.c ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def)

set_entrypoint(beepmidi 0)

add_importlibs(beepmidi winmm kernel32 ntdll)
add_dependencies(beepmidi psdk)
add_cab_target(beepmidi 1)