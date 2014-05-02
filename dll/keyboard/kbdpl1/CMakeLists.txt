
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdpl1.dll kbdpl1.spec)

add_library(kbdpl1 SHARED kbdpl1.c kbdpl1.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdpl1.def)

set_module_type(kbdpl1 kbdlayout)

add_dependencies(kbdpl1 psdk)

add_cd_file(TARGET kbdpl1 DESTINATION reactos/system32 FOR all)
