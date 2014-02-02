
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdcr.dll kbdcr.spec)

add_library(kbdcr SHARED kbdcr.c kbdcr.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdcr.def)

set_module_type(kbdcr kbdlayout)

add_dependencies(kbdcr psdk)

add_cd_file(TARGET kbdcr DESTINATION reactos/system32 FOR all)
