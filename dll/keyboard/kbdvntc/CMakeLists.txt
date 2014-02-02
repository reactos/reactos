
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdvntc.dll kbdvntc.spec)

add_library(kbdvntc SHARED kbdvntc.c kbdvntc.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdvntc.def)

set_module_type(kbdvntc kbdlayout)

add_dependencies(kbdvntc psdk)

add_cd_file(TARGET kbdvntc DESTINATION reactos/system32 FOR all)
