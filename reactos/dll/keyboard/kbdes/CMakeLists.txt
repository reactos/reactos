
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdes.dll kbdes.spec)

add_library(kbdes SHARED kbdes.c kbdes.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdes.def)

set_module_type(kbdes kbdlayout)

add_dependencies(kbdes psdk)

add_cd_file(TARGET kbdes DESTINATION reactos/system32 FOR all)
