
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdcz1.dll kbdcz1.spec)

add_library(kbdcz1 SHARED kbdcz1.c kbdcz1.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdcz1.def)

set_module_type(kbdcz1 kbdlayout)

add_dependencies(kbdcz1 psdk)

add_cd_file(TARGET kbdcz1 DESTINATION reactos/system32 FOR all)
