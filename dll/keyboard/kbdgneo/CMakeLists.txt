
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgneo.dll kbdgneo.spec)

add_library(kbdgneo SHARED kbdgneo.c kbdgneo.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdgneo.def)

set_module_type(kbdgneo kbdlayout)

add_dependencies(kbdgneo psdk)

add_cd_file(TARGET kbdgneo DESTINATION reactos/system32 FOR all)
