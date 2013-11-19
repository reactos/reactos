
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbe.dll kbdbe.spec)

add_library(kbdbe SHARED kbdbe.c kbdbe.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdbe.def)

set_module_type(kbdbe kbdlayout)

add_dependencies(kbdbe psdk)

add_cd_file(TARGET kbdbe DESTINATION reactos/system32 FOR all)
