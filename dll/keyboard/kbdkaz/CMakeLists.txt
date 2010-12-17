
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdkaz.dll kbdkaz.spec)

add_library(kbdkaz SHARED kbdkaz.c kbdkaz.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdkaz.def)

set_entrypoint(kbdkaz 0)

add_dependencies(kbdkaz psdk buildno_header)

add_cab_target(kbdkaz 1)
