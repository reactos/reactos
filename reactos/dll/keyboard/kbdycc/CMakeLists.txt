
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdycc.dll kbdycc.spec)

add_library(kbdycc SHARED kbdycc.c kbdycc.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdycc.def)

set_entrypoint(kbdycc 0)

add_dependencies(kbdycc psdk)

add_cd_file(TARGET kbdycc DESTINATION reactos/system32 FOR all)
