
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdycc ${CMAKE_CURRENT_SOURCE_DIR}/kbdycc.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdycc.def)

add_library(kbdycc SHARED kbdycc.c kbdycc.rc)

set_entrypoint(kbdycc 0)

target_link_libraries(kbdycc ${CMAKE_CURRENT_BINARY_DIR}/kbdycc.def)

add_dependencies(kbdycc kbdycc_def psdk buildno_header)
