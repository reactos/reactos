
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdko ${CMAKE_CURRENT_SOURCE_DIR}/kbdko.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdko.def)

add_library(kbdko SHARED kbdko.c kbdko.rc)

set_entrypoint(kbdko 0)

target_link_libraries(kbdko ${CMAKE_CURRENT_BINARY_DIR}/kbdko.def)

add_dependencies(kbdko kbdko_def psdk buildno_header)
