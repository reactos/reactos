
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdhu.dll kbdhu.spec)

add_library(kbdhu SHARED kbdhu.c kbdhu.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdhu.def)

set_entrypoint(kbdhu 0)

add_dependencies(kbdhu psdk buildno_header)

add_cab_target(kbdhu 1)