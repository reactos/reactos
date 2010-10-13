
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdth3 ${CMAKE_CURRENT_SOURCE_DIR}/kbdth3.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdth3.def)

add_library(kbdth3 SHARED kbdth3.c kbdth3.rc)

set_entrypoint(kbdth3 0)

target_link_libraries(kbdth3 ${CMAKE_CURRENT_BINARY_DIR}/kbdth3.def)

add_dependencies(kbdth3 kbdth3_def psdk buildno_header)
