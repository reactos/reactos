
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdru1.dll kbdru1.spec)

add_library(kbdru1 SHARED kbdru1.c kbdru1.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdru1.def)

set_entrypoint(kbdru1 0)

add_dependencies(kbdru1 psdk buildno_header)

add_cab_target(kbdru1 1)