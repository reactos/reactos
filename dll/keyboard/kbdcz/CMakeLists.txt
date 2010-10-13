
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdcz ${CMAKE_CURRENT_SOURCE_DIR}/kbdcz.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdcz.def)

add_library(kbdcz SHARED kbdcz.c kbdcz.rc)

set_entrypoint(kbdcz 0)

target_link_libraries(kbdcz ${CMAKE_CURRENT_BINARY_DIR}/kbdcz.def)

add_dependencies(kbdcz kbdcz_def psdk buildno_header)
