
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdko.dll kbdko.spec)

add_library(kbdko SHARED kbdko.c kbdko.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdko.def)

set_module_type(kbdko kbdlayout)

add_dependencies(kbdko psdk)

add_cd_file(TARGET kbdko DESTINATION reactos/system32 FOR all)
