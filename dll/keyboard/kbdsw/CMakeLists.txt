
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdsw.dll kbdsw.spec)

add_library(kbdsw SHARED kbdsw.c kbdsw.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdsw.def)

set_module_type(kbdsw kbdlayout)

add_dependencies(kbdsw psdk)

add_cd_file(TARGET kbdsw DESTINATION reactos/system32 FOR all)
