
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdne.dll kbdne.spec)

add_library(kbdne SHARED kbdne.c kbdne.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdne.def)

set_entrypoint(kbdne 0)

add_dependencies(kbdne psdk)

add_cd_file(TARGET kbdne DESTINATION reactos/system32 FOR all)
