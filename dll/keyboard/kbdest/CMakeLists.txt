
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdest.dll kbdest.spec)

add_library(kbdest SHARED kbdest.c kbdest.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdest.def)

set_entrypoint(kbdest 0)

add_dependencies(kbdest psdk)

add_cab_target(kbdest 1)
