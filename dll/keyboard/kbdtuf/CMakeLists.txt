
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdtuf.dll kbdtuf.spec)

add_library(kbdtuf SHARED kbdtuf.c kbdtuf.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdtuf.def)

set_module_type(kbdtuf kbdlayout)

add_dependencies(kbdtuf psdk)

add_cd_file(TARGET kbdtuf DESTINATION reactos/system32 FOR all)
