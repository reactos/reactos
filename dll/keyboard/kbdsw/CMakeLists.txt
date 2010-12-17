
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdsw.dll kbdsw.spec)

add_library(kbdsw SHARED kbdsw.c kbdsw.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdsw.def)

set_entrypoint(kbdsw 0)

add_dependencies(kbdsw psdk buildno_header)

add_cab_target(kbdsw 1)
