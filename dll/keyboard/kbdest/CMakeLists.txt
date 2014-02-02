
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdest.dll kbdest.spec)

add_library(kbdest SHARED kbdest.c kbdest.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdest.def)

set_module_type(kbdest kbdlayout)

add_dependencies(kbdest psdk)

add_cd_file(TARGET kbdest DESTINATION reactos/system32 FOR all)
