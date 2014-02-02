
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdlv.dll kbdlv.spec)

add_library(kbdlv SHARED kbdlv.c kbdlv.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdlv.def)

set_module_type(kbdlv kbdlayout)

add_dependencies(kbdlv psdk)

add_cd_file(TARGET kbdlv DESTINATION reactos/system32 FOR all)
