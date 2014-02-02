
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbgm.dll kbdbgm.spec)

add_library(kbdbgm SHARED kbdbgm.c kbdbgm.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdbgm.def)

set_module_type(kbdbgm kbdlayout)

add_dependencies(kbdbgm psdk)

add_cd_file(TARGET kbdbgm DESTINATION reactos/system32 FOR all)
