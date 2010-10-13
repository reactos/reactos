
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdro ${CMAKE_CURRENT_SOURCE_DIR}/kbdro.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdro.def)

add_library(kbdro SHARED kbdro.c kbdro.rc)

set_entrypoint(kbdro 0)

target_link_libraries(kbdro ${CMAKE_CURRENT_BINARY_DIR}/kbdro.def)

add_dependencies(kbdro kbdro_def psdk buildno_header)
