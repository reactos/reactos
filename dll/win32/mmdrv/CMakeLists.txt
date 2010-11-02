
set_unicode()

add_definitions(-DNDEBUG)

add_library(mmdrv SHARED
    entry.c
    mme.c
    kernel.c
    session.c
    common.c
    wave.c
    wave_io.c
    mmdrv.def)

if(NOT MSVC)
    set_source_files_properties(mmdrv.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(mmdrv 0)

add_importlibs(mmdrv winmm user32 kernel32 ntdll)
add_dependencies(mmdrv psdk buildno_header)
add_cab_target(mmdrv 1)