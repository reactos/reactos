
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdheb.dll kbdheb.spec)

add_library(kbdheb SHARED kbdheb.c kbdheb.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdheb.def)

set_module_type(kbdheb kbdlayout)

add_dependencies(kbdheb psdk)

add_cd_file(TARGET kbdheb DESTINATION reactos/system32 FOR all)
