
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdic.dll kbdic.spec)

add_library(kbdic SHARED kbdic.c kbdic.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdic.def)

set_module_type(kbdic kbdlayout)

add_dependencies(kbdic psdk)

add_cd_file(TARGET kbdic DESTINATION reactos/system32 FOR all)
