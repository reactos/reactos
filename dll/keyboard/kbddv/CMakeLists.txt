
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbddv.dll kbddv.spec)

add_library(kbddv SHARED kbddv.c kbddv.rc ${CMAKE_CURRENT_BINARY_DIR}/kbddv.def)

set_entrypoint(kbddv 0)

add_dependencies(kbddv psdk)

add_cab_target(kbddv 1)
