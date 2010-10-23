
set_unicode()

add_definitions(-DNDEBUG)

spec2def(mmdrv ${CMAKE_CURRENT_SOURCE_DIR}/mmdrv.spec ${CMAKE_CURRENT_BINARY_DIR}/mmdrv.def)

add_library(mmdrv SHARED
    entry.c
    mme.c
    kernel.c
    session.c
    common.c
    wave.c
    wave_io.c)

set_entrypoint(mmdrv 0)

target_link_libraries(mmdrv ${CMAKE_CURRENT_BINARY_DIR}/mmdrv.def)

add_importlibs(mmdrv winmm user32 kernel32 ntdll)
add_dependencies(mmdrv mmdrv_def psdk buildno_header)
add_livecd_target(mmdrv reactos/system32)