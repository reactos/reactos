
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbgt.dll kbdbgt.spec)

add_library(kbdbgt SHARED kbdbgt.c kbdbgt.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdbgt.def)

set_module_type(kbdbgt kbdlayout)

add_dependencies(kbdbgt psdk)

add_cd_file(TARGET kbdbgt DESTINATION reactos/system32 FOR all)
