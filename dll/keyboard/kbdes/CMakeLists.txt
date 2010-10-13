
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdes ${CMAKE_CURRENT_SOURCE_DIR}/kbdes.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdes.def)

add_library(kbdes SHARED kbdes.c kbdes.rc)

set_entrypoint(kbdes 0)

target_link_libraries(kbdes ${CMAKE_CURRENT_BINARY_DIR}/kbdes.def)

add_dependencies(kbdes kbdes_def psdk buildno_header)
