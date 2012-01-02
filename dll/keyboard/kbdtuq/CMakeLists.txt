
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdtuq.dll kbdtuq.spec)

add_library(kbdtuq SHARED kbdtuq.c kbdtuq.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdtuq.def)

set_entrypoint(kbdtuq 0)

add_dependencies(kbdtuq psdk)

add_cd_file(TARGET kbdtuq DESTINATION reactos/system32 FOR all)
