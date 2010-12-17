
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdheb.dll kbdheb.spec)

add_library(kbdheb SHARED kbdheb.c kbdheb.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdheb.def)

set_entrypoint(kbdheb 0)

add_dependencies(kbdheb psdk buildno_header)

add_cab_target(kbdheb 1)
