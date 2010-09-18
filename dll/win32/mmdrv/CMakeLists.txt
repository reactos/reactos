


add_definitions(-DNDEBUG)
add_definitions(-DUNICODE -D_UNICODE)

spec2def(mmdrv ${CMAKE_CURRENT_SOURCE_DIR}/mmdrv.spec ${CMAKE_CURRENT_BINARY_DIR}/mmdrv.def)

add_library(mmdrv SHARED
    entry.c
    mme.c
    kernel.c
    session.c
    common.c
    wave.c
    wave_io.c)

set_target_properties(mmdrv PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(mmdrv
    ${CMAKE_CURRENT_BINARY_DIR}/mmdrv.def
    -lwinmm
    -luser32
    -lkernel32)

add_dependencies(mmdrv mmdrv_def psdk)
