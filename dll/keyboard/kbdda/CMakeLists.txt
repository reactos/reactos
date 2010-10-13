
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdda ${CMAKE_CURRENT_SOURCE_DIR}/kbdda.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdda.def)

add_library(kbdda SHARED kbdda.c kbdda.rc)

set_entrypoint(kbdda 0)

target_link_libraries(kbdda ${CMAKE_CURRENT_BINARY_DIR}/kbdda.def)

add_dependencies(kbdda kbdda_def psdk buildno_header)
