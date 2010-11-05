
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdvntc.dll kbdvntc.spec)

add_library(kbdvntc SHARED kbdvntc.c kbdvntc.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdvntc.def)

set_entrypoint(kbdvntc 0)

add_dependencies(kbdvntc psdk buildno_header)

add_cab_target(kbdvntc 1)