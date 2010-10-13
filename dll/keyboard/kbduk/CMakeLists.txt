
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbduk ${CMAKE_CURRENT_SOURCE_DIR}/kbduk.spec ${CMAKE_CURRENT_BINARY_DIR}/kbduk.def)

add_library(kbduk SHARED kbduk.c kbduk.rc)

set_entrypoint(kbduk 0)

target_link_libraries(kbduk ${CMAKE_CURRENT_BINARY_DIR}/kbduk.def)

add_dependencies(kbduk kbduk_def psdk buildno_header)
