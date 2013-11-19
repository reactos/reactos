
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdla.dll kbdla.spec)

add_library(kbdla SHARED kbdla.c kbdla.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdla.def)

set_module_type(kbdla kbdlayout)

add_dependencies(kbdla psdk)

add_cd_file(TARGET kbdla DESTINATION reactos/system32 FOR all)
