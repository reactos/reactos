
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdur.dll kbdur.spec)

add_library(kbdur SHARED kbdur.c kbdur.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdur.def)

set_module_type(kbdur kbdlayout)

add_dependencies(kbdur psdk)

add_cd_file(TARGET kbdur DESTINATION reactos/system32 FOR all)
