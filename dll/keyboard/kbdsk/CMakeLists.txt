
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdsk.dll kbdsk.spec)

add_library(kbdsk SHARED kbdsk.c kbdsk.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdsk.def)

set_module_type(kbdsk kbdlayout)

add_dependencies(kbdsk psdk)

add_cd_file(TARGET kbdsk DESTINATION reactos/system32 FOR all)
