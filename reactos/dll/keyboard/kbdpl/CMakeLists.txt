
spec2def(kbdpl.dll kbdpl.spec)

add_library(kbdpl SHARED kbdpl.c kbdpl.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdpl.def)

set_entrypoint(kbdpl 0)

add_dependencies(kbdpl psdk)

add_cd_file(TARGET kbdpl DESTINATION reactos/system32 FOR all)
