
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdir.dll kbdir.spec)

add_library(kbdir SHARED kbdir.c kbdir.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdir.def)

set_module_type(kbdir kbdlayout)

add_dependencies(kbdir psdk)

add_cd_file(TARGET kbdir DESTINATION reactos/system32 FOR all)
