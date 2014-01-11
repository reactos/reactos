
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdcz.dll kbdcz.spec)

add_library(kbdcz SHARED kbdcz.c kbdcz.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdcz.def)

set_module_type(kbdcz kbdlayout)

add_dependencies(kbdcz psdk)

add_cd_file(TARGET kbdcz DESTINATION reactos/system32 FOR all)
