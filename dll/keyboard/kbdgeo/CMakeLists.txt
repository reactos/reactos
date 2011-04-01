
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgeo.dll kbdgeo.spec)

add_library(kbdgeo SHARED kbdgeo.c kbdgeo.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdgeo.def)

set_entrypoint(kbdgeo 0)

add_dependencies(kbdgeo psdk)

add_cab_target(kbdgeo 1)
