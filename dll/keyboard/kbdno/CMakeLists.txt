
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdno ${CMAKE_CURRENT_SOURCE_DIR}/kbdno.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdno.def)

add_library(kbdno SHARED kbdno.c kbdno.rc)

set_entrypoint(kbdno 0)

target_link_libraries(kbdno ${CMAKE_CURRENT_BINARY_DIR}/kbdno.def)

add_dependencies(kbdno kbdno_def psdk buildno_header)
