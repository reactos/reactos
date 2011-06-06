
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdhe.dll kbdhe.spec)

add_library(kbdhe SHARED kbdhe.c kbdhe.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdhe.def)

set_entrypoint(kbdhe 0)

add_dependencies(kbdhe psdk)

add_cd_file(TARGET kbdhe DESTINATION reactos/system32 FOR all)
