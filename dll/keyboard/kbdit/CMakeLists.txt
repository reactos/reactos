
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdit.dll kbdit.spec)

add_library(kbdit SHARED kbdit.c kbdit.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdit.def)

set_module_type(kbdit kbdlayout)

add_dependencies(kbdit psdk)

add_cd_file(TARGET kbdit DESTINATION reactos/system32 FOR all)
