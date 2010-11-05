
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdur.dll kbdur.spec)

add_library(kbdur SHARED kbdur.c kbdur.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdur.def)

set_entrypoint(kbdur 0)

add_dependencies(kbdur psdk buildno_header)

add_cab_target(kbdur 1)