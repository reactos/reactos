
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdarmw.dll kbdarmw.spec)

add_library(kbdarmw SHARED kbdarmw.c kbdarmw.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdarmw.def)

set_entrypoint(kbdarmw 0)

add_dependencies(kbdarmw psdk buildno_header)

add_cab_target(kbdarmw 1)
