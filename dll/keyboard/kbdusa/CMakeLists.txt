
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusa.dll kbdusa.spec)

add_library(kbdusa SHARED kbdusa.c kbdusa.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdusa.def)

set_entrypoint(kbdusa 0)

add_dependencies(kbdusa psdk)

add_cab_target(kbdusa 1)
