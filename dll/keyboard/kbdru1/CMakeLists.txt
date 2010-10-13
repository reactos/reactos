
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdru1 ${CMAKE_CURRENT_SOURCE_DIR}/kbdru1.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdru1.def)

add_library(kbdru1 SHARED kbdru1.c kbdru1.rc)

set_entrypoint(kbdru1 0)

target_link_libraries(kbdru1 ${CMAKE_CURRENT_BINARY_DIR}/kbdru1.def)

add_dependencies(kbdru1 kbdru1_def psdk buildno_header)
