
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdda.dll kbdda.spec)

add_library(kbdda SHARED kbdda.c kbdda.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdda.def)

set_entrypoint(kbdda 0)

add_dependencies(kbdda psdk buildno_header)

add_cab_target(kbdda 1)
