
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbddv.dll kbddv.spec)

add_library(kbddv SHARED kbddv.c kbddv.rc ${CMAKE_CURRENT_BINARY_DIR}/kbddv.def)

set_module_type(kbddv kbdlayout)

add_dependencies(kbddv psdk)

add_cd_file(TARGET kbddv DESTINATION reactos/system32 FOR all)
