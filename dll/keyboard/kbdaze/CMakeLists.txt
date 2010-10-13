
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdaze ${CMAKE_CURRENT_SOURCE_DIR}/kbdaze.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdaze.def)

add_library(kbdaze SHARED kbdaze.c kbdaze.rc)

set_entrypoint(kbdaze 0)

target_link_libraries(kbdaze ${CMAKE_CURRENT_BINARY_DIR}/kbdaze.def)

add_dependencies(kbdaze kbdaze_def psdk buildno_header)
