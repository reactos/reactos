
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbduk.dll kbduk.spec)

add_library(kbduk SHARED kbduk.c kbduk.rc ${CMAKE_CURRENT_BINARY_DIR}/kbduk.def)

set_entrypoint(kbduk 0)

add_dependencies(kbduk psdk)

add_cab_target(kbduk 1)
