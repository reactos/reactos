
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbduzb ${CMAKE_CURRENT_SOURCE_DIR}/kbduzb.spec ${CMAKE_CURRENT_BINARY_DIR}/kbduzb.def)

add_library(kbduzb SHARED kbduzb.c kbduzb.rc)

set_entrypoint(kbduzb 0)

target_link_libraries(kbduzb ${CMAKE_CURRENT_BINARY_DIR}/kbduzb.def)

add_dependencies(kbduzb kbduzb_def psdk buildno_header)
