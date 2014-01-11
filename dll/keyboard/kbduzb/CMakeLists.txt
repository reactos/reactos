
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbduzb.dll kbduzb.spec)

add_library(kbduzb SHARED kbduzb.c kbduzb.rc ${CMAKE_CURRENT_BINARY_DIR}/kbduzb.def)

set_module_type(kbduzb kbdlayout)

add_dependencies(kbduzb psdk)

add_cd_file(TARGET kbduzb DESTINATION reactos/system32 FOR all)
