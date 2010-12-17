
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbr.dll kbdbr.spec)

add_library(kbdbr SHARED kbdbr.c kbdbr.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdbr.def)

set_entrypoint(kbdbr 0)

add_dependencies(kbdbr psdk buildno_header)

add_cab_target(kbdbr 1)
