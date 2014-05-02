
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdro.dll kbdro.spec)

add_library(kbdro SHARED kbdro.c kbdro.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdro.def)

set_module_type(kbdro kbdlayout)

add_dependencies(kbdro psdk)

add_cd_file(TARGET kbdro DESTINATION reactos/system32 FOR all)
