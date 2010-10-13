
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdth1 ${CMAKE_CURRENT_SOURCE_DIR}/kbdth1.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdth1.def)

add_library(kbdth1 SHARED kbdth1.c kbdth1.rc)

set_entrypoint(kbdth1 0)

target_link_libraries(kbdth1 ${CMAKE_CURRENT_BINARY_DIR}/kbdth1.def)

add_dependencies(kbdth1 kbdth1_def psdk buildno_header)
