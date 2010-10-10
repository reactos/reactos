
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dplay ${CMAKE_CURRENT_SOURCE_DIR}/dplay.spec ${CMAKE_CURRENT_BINARY_DIR}/dplay.def)

add_library(dplay SHARED dplay_main.c version.rc)

set_entrypoint(dplay 0)

target_link_libraries(dplay
    ${CMAKE_CURRENT_BINARY_DIR}/dplay.def
    uuid
    wine)

add_importlibs(dplay
    user32
    advapi32
    ole32
    winmm
    dinput
    ntdll)

add_dependencies(dplay dplay_def psdk buildno_header)
