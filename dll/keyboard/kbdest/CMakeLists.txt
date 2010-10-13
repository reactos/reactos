
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdest ${CMAKE_CURRENT_SOURCE_DIR}/kbdest.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdest.def)

add_library(kbdest SHARED kbdest.c kbdest.rc)

set_entrypoint(kbdest 0)

target_link_libraries(kbdest ${CMAKE_CURRENT_BINARY_DIR}/kbdest.def)

add_dependencies(kbdest kbdest_def psdk buildno_header)
