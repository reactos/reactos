
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdtat.dll kbdtat.spec)

add_library(kbdtat SHARED kbdtat.c kbdtat.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdtat.def)

set_module_type(kbdtat kbdlayout)

add_dependencies(kbdtat psdk)

add_cd_file(TARGET kbdtat DESTINATION reactos/system32 FOR all)
