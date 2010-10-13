
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdsw ${CMAKE_CURRENT_SOURCE_DIR}/kbdsw.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdsw.def)

add_library(kbdsw SHARED kbdsw.c kbdsw.rc)

set_entrypoint(kbdsw 0)

target_link_libraries(kbdsw ${CMAKE_CURRENT_BINARY_DIR}/kbdsw.def)

add_dependencies(kbdsw kbdsw_def psdk buildno_header)
