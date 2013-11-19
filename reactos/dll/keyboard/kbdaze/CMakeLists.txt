
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdaze.dll kbdaze.spec)

add_library(kbdaze SHARED kbdaze.c kbdaze.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdaze.def)

set_module_type(kbdaze kbdlayout)

add_dependencies(kbdaze psdk)

add_cd_file(TARGET kbdaze DESTINATION reactos/system32 FOR all)
