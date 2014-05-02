
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdarmw.dll kbdarmw.spec)

add_library(kbdarmw SHARED kbdarmw.c kbdarmw.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdarmw.def)

set_module_type(kbdarmw kbdlayout)

add_dependencies(kbdarmw psdk)

add_cd_file(TARGET kbdarmw DESTINATION reactos/system32 FOR all)
