
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdinasa.dll kbdinasa.spec)

add_library(kbdinasa SHARED kbdinasa.c kbdinasa.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdinasa.def)

set_module_type(kbdinasa kbdlayout)

add_dependencies(kbdinasa psdk)

add_cd_file(TARGET kbdinasa DESTINATION reactos/system32 FOR all)
