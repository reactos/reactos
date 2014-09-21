
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdda.dll kbdda.spec)

add_library(kbdda SHARED kbdda.c kbdda.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdda.def)

set_module_type(kbdda kbdlayout)

add_dependencies(kbdda psdk)

add_cd_file(TARGET kbdda DESTINATION reactos/system32 FOR all)
