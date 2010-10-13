
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdsk1 ${CMAKE_CURRENT_SOURCE_DIR}/kbdsk1.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdsk1.def)

add_library(kbdsk1 SHARED kbdsk1.c kbdsk1.rc)

set_entrypoint(kbdsk1 0)

target_link_libraries(kbdsk1 ${CMAKE_CURRENT_BINARY_DIR}/kbdsk1.def)

add_dependencies(kbdsk1 kbdsk1_def psdk buildno_header)
