
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdinguj ${CMAKE_CURRENT_SOURCE_DIR}/kbdinguj.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdinguj.def)

add_library(kbdinguj SHARED kbdinguj.c kbdinguj.rc)

set_entrypoint(kbdinguj 0)

target_link_libraries(kbdinguj ${CMAKE_CURRENT_BINARY_DIR}/kbdinguj.def)

add_dependencies(kbdinguj kbdinguj_def psdk buildno_header)
