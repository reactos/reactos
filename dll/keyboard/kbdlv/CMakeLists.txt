
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdlv ${CMAKE_CURRENT_SOURCE_DIR}/kbdlv.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdlv.def)

add_library(kbdlv SHARED kbdlv.c kbdlv.rc)

set_entrypoint(kbdlv 0)

target_link_libraries(kbdlv ${CMAKE_CURRENT_BINARY_DIR}/kbdlv.def)

add_dependencies(kbdlv kbdlv_def psdk buildno_header)
