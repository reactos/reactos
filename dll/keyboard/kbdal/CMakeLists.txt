
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdal ${CMAKE_CURRENT_SOURCE_DIR}/kbdal.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdal.def)

add_library(kbdal SHARED kbdal.c kbdal.rc)

set_entrypoint(kbdal 0)

target_link_libraries(kbdal ${CMAKE_CURRENT_BINARY_DIR}/kbdal.def)

add_dependencies(kbdal kbdal_def psdk buildno_header)
