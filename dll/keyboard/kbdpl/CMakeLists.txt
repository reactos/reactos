
spec2def(kbdpl.dll kbdpl.spec)

add_library(kbdpl SHARED kbdpl.c kbdpl.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdpl.def)

set_module_type(kbdpl kbdlayout)

add_dependencies(kbdpl psdk)

add_cd_file(TARGET kbdpl DESTINATION reactos/system32 FOR all)
