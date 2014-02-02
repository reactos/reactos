
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdcan.dll kbdcan.spec)

add_library(kbdcan SHARED kbdcan.c kbdcan.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdcan.def)

set_module_type(kbdcan kbdlayout)

add_dependencies(kbdcan psdk)

add_cd_file(TARGET kbdcan DESTINATION reactos/system32 FOR all)
