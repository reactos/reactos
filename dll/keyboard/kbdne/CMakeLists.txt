
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdne ${CMAKE_CURRENT_SOURCE_DIR}/kbdne.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdne.def)

add_library(kbdne SHARED kbdne.c kbdne.rc)

set_entrypoint(kbdne 0)

target_link_libraries(kbdne ${CMAKE_CURRENT_BINARY_DIR}/kbdne.def)

add_dependencies(kbdne kbdne_def psdk buildno_header)
