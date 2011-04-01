
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdtat.dll kbdtat.spec)

add_library(kbdtat SHARED kbdtat.c kbdtat.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdtat.def)

set_entrypoint(kbdtat 0)

add_dependencies(kbdtat psdk)

add_cab_target(kbdtat 1)
