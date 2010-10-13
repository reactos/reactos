
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgrist ${CMAKE_CURRENT_SOURCE_DIR}/kbdgrist.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdgrist.def)

add_library(kbdgrist SHARED kbdgrist.c kbdgrist.rc)

set_entrypoint(kbdgrist 0)

target_link_libraries(kbdgrist ${CMAKE_CURRENT_BINARY_DIR}/kbdgrist.def)

add_dependencies(kbdgrist kbdgrist_def psdk buildno_header)
