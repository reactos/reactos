
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdsk1.dll kbdsk1.spec)

add_library(kbdsk1 SHARED kbdsk1.c kbdsk1.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdsk1.def)

set_module_type(kbdsk1 kbdlayout)

add_dependencies(kbdsk1 psdk)

add_cd_file(TARGET kbdsk1 DESTINATION reactos/system32 FOR all)
