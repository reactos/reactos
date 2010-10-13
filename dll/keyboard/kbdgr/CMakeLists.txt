
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgr ${CMAKE_CURRENT_SOURCE_DIR}/kbdgr.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdgr.def)

add_library(kbdgr SHARED kbdgr.c kbdgr.rc)

set_entrypoint(kbdgr 0)

target_link_libraries(kbdgr ${CMAKE_CURRENT_BINARY_DIR}/kbdgr.def)

add_dependencies(kbdgr kbdgr_def psdk buildno_header)
