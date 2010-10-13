
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbe ${CMAKE_CURRENT_SOURCE_DIR}/kbdbe.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdbe.def)

add_library(kbdbe SHARED kbdbe.c kbdbe.rc)

set_entrypoint(kbdbe 0)

target_link_libraries(kbdbe ${CMAKE_CURRENT_BINARY_DIR}/kbdbe.def)

add_dependencies(kbdbe kbdbe_def psdk buildno_header)
