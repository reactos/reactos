
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdinben.dll kbdinben.spec)

add_library(kbdinben SHARED kbdinben.c kbdinben.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdinben.def)

set_module_type(kbdinben kbdlayout)

add_dependencies(kbdinben psdk)

add_cd_file(TARGET kbdinben DESTINATION reactos/system32 FOR all)
