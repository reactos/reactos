
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdro.dll kbdro.spec)

add_library(kbdro SHARED kbdro.c kbdro.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdro.def)

set_entrypoint(kbdro 0)

add_dependencies(kbdro psdk buildno_header)

add_cab_target(kbdro 1)