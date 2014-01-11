
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdru.dll kbdru.spec)

add_library(kbdru SHARED kbdru.c kbdru.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdru.def)

set_module_type(kbdru kbdlayout)

add_dependencies(kbdru psdk)

add_cd_file(TARGET kbdru DESTINATION reactos/system32 FOR all)
