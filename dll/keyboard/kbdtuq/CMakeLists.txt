
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdtuq.dll kbdtuq.spec)

add_library(kbdtuq SHARED kbdtuq.c kbdtuq.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdtuq.def)

set_entrypoint(kbdtuq 0)

add_dependencies(kbdtuq psdk buildno_header)

add_cab_target(kbdtuq 1)