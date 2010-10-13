
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdtat ${CMAKE_CURRENT_SOURCE_DIR}/kbdtat.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdtat.def)

add_library(kbdtat SHARED kbdtat.c kbdtat.rc)

set_entrypoint(kbdtat 0)

target_link_libraries(kbdtat ${CMAKE_CURRENT_BINARY_DIR}/kbdtat.def)

add_dependencies(kbdtat kbdtat_def psdk buildno_header)
