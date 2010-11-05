
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbe.dll kbdbe.spec)

add_library(kbdbe SHARED kbdbe.c kbdbe.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdbe.def)

set_entrypoint(kbdbe 0)

add_dependencies(kbdbe psdk buildno_header)

add_cab_target(kbdbe 1)