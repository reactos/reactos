
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbr.dll kbdbr.spec)

add_library(kbdbr SHARED kbdbr.c kbdbr.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdbr.def)

set_module_type(kbdbr kbdlayout)

add_dependencies(kbdbr psdk)

add_cd_file(TARGET kbdbr DESTINATION reactos/system32 FOR all)
