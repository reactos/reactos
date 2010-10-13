
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdcr ${CMAKE_CURRENT_SOURCE_DIR}/kbdcr.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdcr.def)

add_library(kbdcr SHARED kbdcr.c kbdcr.rc)

set_entrypoint(kbdcr 0)

target_link_libraries(kbdcr ${CMAKE_CURRENT_BINARY_DIR}/kbdcr.def)

add_dependencies(kbdcr kbdcr_def psdk buildno_header)
