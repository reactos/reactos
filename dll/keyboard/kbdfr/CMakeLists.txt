
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdfr.dll kbdfr.spec)

add_library(kbdfr SHARED kbdfr.c kbdfr.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdfr.def)

set_module_type(kbdfr kbdlayout)

add_dependencies(kbdfr psdk)

add_cd_file(TARGET kbdfr DESTINATION reactos/system32 FOR all)
