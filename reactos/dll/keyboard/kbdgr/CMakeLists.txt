
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgr.dll kbdgr.spec)

add_library(kbdgr SHARED kbdgr.c kbdgr.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdgr.def)

set_module_type(kbdgr kbdlayout)

add_dependencies(kbdgr psdk)

add_cd_file(TARGET kbdgr DESTINATION reactos/system32 FOR all)
