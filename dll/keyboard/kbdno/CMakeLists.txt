
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdno.dll kbdno.spec)

add_library(kbdno SHARED kbdno.c kbdno.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdno.def)

set_module_type(kbdno kbdlayout)

add_dependencies(kbdno psdk)

add_cd_file(TARGET kbdno DESTINATION reactos/system32 FOR all)
