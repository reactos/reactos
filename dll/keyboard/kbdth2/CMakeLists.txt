
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdth2 ${CMAKE_CURRENT_SOURCE_DIR}/kbdth2.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdth2.def)

add_library(kbdth2 SHARED kbdth2.c kbdth2.rc)

set_entrypoint(kbdth2 0)

target_link_libraries(kbdth2 ${CMAKE_CURRENT_BINARY_DIR}/kbdth2.def)

add_dependencies(kbdth2 kbdth2_def psdk buildno_header)
