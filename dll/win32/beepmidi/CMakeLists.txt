
set_unicode()

add_library(beepmidi SHARED beepmidi.c beepmidi.def)

if(NOT MSVC)
    set_source_files_properties(beepmidi.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(beepmidi 0)

add_importlibs(beepmidi winmm kernel32 ntdll)
add_dependencies(beepmidi psdk)
add_cab_target(beepmidi 1)