
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdru ${CMAKE_CURRENT_SOURCE_DIR}/kbdru.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdru.def)

add_library(kbdru SHARED kbdru.c kbdru.rc)

set_entrypoint(kbdru 0)

target_link_libraries(kbdru ${CMAKE_CURRENT_BINARY_DIR}/kbdru.def)

add_dependencies(kbdru kbdru_def psdk buildno_header)
