
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusa.dll kbdusa.spec)

add_library(kbdusa SHARED kbdusa.c kbdusa.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdusa.def)

set_module_type(kbdusa kbdlayout)

add_dependencies(kbdusa psdk)

add_cd_file(TARGET kbdusa DESTINATION reactos/system32 FOR all)
