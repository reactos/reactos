
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbga ${CMAKE_CURRENT_SOURCE_DIR}/kbdbga.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdbga.def)

add_library(kbdbga SHARED kbdbga.c kbdbga.rc)

set_entrypoint(kbdbga 0)

target_link_libraries(kbdbga ${CMAKE_CURRENT_BINARY_DIR}/kbdbga.def)

add_dependencies(kbdbga kbdbga_def psdk buildno_header)
