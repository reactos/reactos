
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgerg ${CMAKE_CURRENT_SOURCE_DIR}/kbdgerg.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdgerg.def)

add_library(kbdgerg SHARED kbdgerg.c kbdgerg.rc)

set_entrypoint(kbdgerg 0)

target_link_libraries(kbdgerg ${CMAKE_CURRENT_BINARY_DIR}/kbdgerg.def)

add_dependencies(kbdgerg kbdgerg_def psdk buildno_header)
