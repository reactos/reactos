
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdla.dll kbdla.spec)

add_library(kbdla SHARED kbdla.c kbdla.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdla.def)

set_entrypoint(kbdla 0)

add_dependencies(kbdla psdk buildno_header)

add_cab_target(kbdla 1)
