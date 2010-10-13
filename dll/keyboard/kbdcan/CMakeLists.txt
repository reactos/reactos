
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdcan ${CMAKE_CURRENT_SOURCE_DIR}/kbdcan.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdcan.def)

add_library(kbdcan SHARED kbdcan.c kbdcan.rc)

set_entrypoint(kbdcan 0)

target_link_libraries(kbdcan ${CMAKE_CURRENT_BINARY_DIR}/kbdcan.def)

add_dependencies(kbdcan kbdcan_def psdk buildno_header)
