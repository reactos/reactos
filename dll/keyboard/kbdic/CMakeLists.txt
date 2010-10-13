
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdic ${CMAKE_CURRENT_SOURCE_DIR}/kbdic.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdic.def)

add_library(kbdic SHARED kbdic.c kbdic.rc)

set_entrypoint(kbdic 0)

target_link_libraries(kbdic ${CMAKE_CURRENT_BINARY_DIR}/kbdic.def)

add_dependencies(kbdic kbdic_def psdk buildno_header)
