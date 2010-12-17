
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgr.dll kbdgr.spec)

add_library(kbdgr SHARED kbdgr.c kbdgr.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdgr.def)

set_entrypoint(kbdgr 0)

add_dependencies(kbdgr psdk buildno_header)

add_cab_target(kbdgr 1)
