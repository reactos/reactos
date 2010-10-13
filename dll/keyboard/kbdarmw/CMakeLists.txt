
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdarmw ${CMAKE_CURRENT_SOURCE_DIR}/kbdarmw.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdarmw.def)

add_library(kbdarmw SHARED kbdarmw.c kbdarmw.rc)

set_entrypoint(kbdarmw 0)

target_link_libraries(kbdarmw ${CMAKE_CURRENT_BINARY_DIR}/kbdarmw.def)

add_dependencies(kbdarmw kbdarmw_def psdk buildno_header)
