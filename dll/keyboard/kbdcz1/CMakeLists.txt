
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdcz1 ${CMAKE_CURRENT_SOURCE_DIR}/kbdcz1.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdcz1.def)

add_library(kbdcz1 SHARED kbdcz1.c kbdcz1.rc)

set_entrypoint(kbdcz1 0)

target_link_libraries(kbdcz1 ${CMAKE_CURRENT_BINARY_DIR}/kbdcz1.def)

add_dependencies(kbdcz1 kbdcz1_def psdk buildno_header)
