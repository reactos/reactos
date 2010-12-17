
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdaze.dll kbdaze.spec)

add_library(kbdaze SHARED kbdaze.c kbdaze.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdaze.def)

set_entrypoint(kbdaze 0)

add_dependencies(kbdaze psdk buildno_header)

add_cab_target(kbdaze 1)
