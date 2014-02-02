
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdth3.dll kbdth3.spec)

add_library(kbdth3 SHARED kbdth3.c kbdth3.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdth3.def)

set_module_type(kbdth3 kbdlayout)

add_dependencies(kbdth3 psdk)

add_cd_file(TARGET kbdth3 DESTINATION reactos/system32 FOR all)
