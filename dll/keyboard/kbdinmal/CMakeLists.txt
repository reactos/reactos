
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdinmal.dll kbdinmal.spec)

add_library(kbdinmal SHARED kbdinmal.c kbdinmal.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdinmal.def)

set_module_type(kbdinmal kbdlayout)

add_dependencies(kbdinmal psdk)

add_cd_file(TARGET kbdinmal DESTINATION reactos/system32 FOR all)
