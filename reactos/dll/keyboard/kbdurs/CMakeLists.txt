
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdurs.dll kbdurs.spec)

add_library(kbdurs SHARED kbdurs.c kbdurs.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdurs.def)

set_module_type(kbdurs kbdlayout)

add_dependencies(kbdurs psdk)

add_cd_file(TARGET kbdurs DESTINATION reactos/system32 FOR all)
