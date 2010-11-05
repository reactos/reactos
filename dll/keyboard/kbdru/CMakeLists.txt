
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdru.dll kbdru.spec)

add_library(kbdru SHARED kbdru.c kbdru.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdru.def)

set_entrypoint(kbdru 0)

add_dependencies(kbdru psdk buildno_header)

add_cab_target(kbdru 1)