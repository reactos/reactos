
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdru1.dll kbdru1.spec)

add_library(kbdru1 SHARED kbdru1.c kbdru1.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdru1.def)

set_module_type(kbdru1 kbdlayout)

add_dependencies(kbdru1 psdk)

add_cd_file(TARGET kbdru1 DESTINATION reactos/system32 FOR all)
