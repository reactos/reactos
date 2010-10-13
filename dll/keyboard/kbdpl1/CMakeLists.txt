
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdpl1 ${CMAKE_CURRENT_SOURCE_DIR}/kbdpl1.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdpl1.def)

add_library(kbdpl1 SHARED kbdpl1.c kbdpl1.rc)

set_entrypoint(kbdpl1 0)

target_link_libraries(kbdpl1 ${CMAKE_CURRENT_BINARY_DIR}/kbdpl1.def)

add_dependencies(kbdpl1 kbdpl1_def psdk buildno_header)
