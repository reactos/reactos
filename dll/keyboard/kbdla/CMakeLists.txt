
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdla ${CMAKE_CURRENT_SOURCE_DIR}/kbdla.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdla.def)

add_library(kbdla SHARED kbdla.c kbdla.rc)

set_entrypoint(kbdla 0)

target_link_libraries(kbdla ${CMAKE_CURRENT_BINARY_DIR}/kbdla.def)

add_dependencies(kbdla kbdla_def psdk buildno_header)
