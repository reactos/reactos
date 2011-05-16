
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdblr.dll kbdblr.spec)

add_library(kbdblr SHARED kbdblr.c kbdblr.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdblr.def)

set_entrypoint(kbdblr 0)

add_dependencies(kbdblr psdk)

add_cd_file(TARGET kbdblr DESTINATION reactos/system32 FOR all)
