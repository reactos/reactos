
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdal.dll kbdal.spec)

add_library(kbdal SHARED kbdal.c kbdal.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdal.def)

set_entrypoint(kbdal 0)

add_dependencies(kbdal psdk buildno_header)

add_cab_target(kbdal 1)