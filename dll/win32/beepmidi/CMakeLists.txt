
set_unicode()

add_library(beepmidi SHARED beepmidi.c)

set_pdef_file(beepmidi beepmidi.pdef)
add_importlib_target(${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def)

set_entrypoint(beepmidi 0)

add_importlibs(beepmidi winmm kernel32 ntdll)
add_dependencies(beepmidi psdk)
add_cab_target(beepmidi 1)