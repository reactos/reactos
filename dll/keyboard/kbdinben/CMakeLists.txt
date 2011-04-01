
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdinben.dll kbdinben.spec)

add_library(kbdinben SHARED kbdinben.c kbdinben.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdinben.def)

set_entrypoint(kbdinben 0)

add_dependencies(kbdinben psdk)

add_cab_target(kbdinben 1)
