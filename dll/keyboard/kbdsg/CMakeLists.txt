
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdsg.dll kbdsg.spec)

add_library(kbdsg SHARED kbdsg.c kbdsg.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdsg.def)

set_module_type(kbdsg kbdlayout)

add_dependencies(kbdsg psdk)

add_cd_file(TARGET kbdsg DESTINATION reactos/system32 FOR all)
