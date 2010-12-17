
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdic.dll kbdic.spec)

add_library(kbdic SHARED kbdic.c kbdic.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdic.def)

set_entrypoint(kbdic 0)

add_dependencies(kbdic psdk buildno_header)

add_cab_target(kbdic 1)
