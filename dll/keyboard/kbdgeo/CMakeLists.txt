
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgeo ${CMAKE_CURRENT_SOURCE_DIR}/kbdgeo.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdgeo.def)

add_library(kbdgeo SHARED kbdgeo.c kbdgeo.rc)

set_entrypoint(kbdgeo 0)

target_link_libraries(kbdgeo ${CMAKE_CURRENT_BINARY_DIR}/kbdgeo.def)

add_dependencies(kbdgeo kbdgeo_def psdk buildno_header)
