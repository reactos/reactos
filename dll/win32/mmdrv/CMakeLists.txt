
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

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
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a)

add_dependencies(mmdrv mmdrv_def psdk)
