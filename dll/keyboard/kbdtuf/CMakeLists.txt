
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdtuf.dll kbdtuf.spec)

add_library(kbdtuf SHARED kbdtuf.c kbdtuf.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdtuf.def)

set_entrypoint(kbdtuf 0)

add_dependencies(kbdtuf psdk buildno_header)

add_cab_target(kbdtuf 1)
