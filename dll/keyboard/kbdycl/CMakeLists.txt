
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdycl.dll kbdycl.spec)

add_library(kbdycl SHARED kbdycl.c kbdycl.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdycl.def)

set_entrypoint(kbdycl 0)

add_dependencies(kbdycl psdk)

add_cab_target(kbdycl 1)
