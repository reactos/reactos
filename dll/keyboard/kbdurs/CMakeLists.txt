
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdurs ${CMAKE_CURRENT_SOURCE_DIR}/kbdurs.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdurs.def)

add_library(kbdurs SHARED kbdurs.c kbdurs.rc)

set_entrypoint(kbdurs 0)

target_link_libraries(kbdurs ${CMAKE_CURRENT_BINARY_DIR}/kbdurs.def)

add_dependencies(kbdurs kbdurs_def psdk buildno_header)
