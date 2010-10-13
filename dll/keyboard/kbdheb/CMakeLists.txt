
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdheb ${CMAKE_CURRENT_SOURCE_DIR}/kbdheb.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdheb.def)

add_library(kbdheb SHARED kbdheb.c kbdheb.rc)

set_entrypoint(kbdheb 0)

target_link_libraries(kbdheb ${CMAKE_CURRENT_BINARY_DIR}/kbdheb.def)

add_dependencies(kbdheb kbdheb_def psdk buildno_header)
