
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdinguj.dll kbdinguj.spec)

add_library(kbdinguj SHARED kbdinguj.c kbdinguj.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdinguj.def)

set_module_type(kbdinguj kbdlayout)

add_dependencies(kbdinguj psdk)

add_cd_file(TARGET kbdinguj DESTINATION reactos/system32 FOR all)
