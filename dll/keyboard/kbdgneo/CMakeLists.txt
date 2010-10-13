
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgneo ${CMAKE_CURRENT_SOURCE_DIR}/kbdgneo.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdgneo.def)

add_library(kbdgneo SHARED kbdgneo.c kbdgneo.rc)

set_entrypoint(kbdgneo 0)

target_link_libraries(kbdgneo ${CMAKE_CURRENT_BINARY_DIR}/kbdgneo.def)

add_dependencies(kbdgneo kbdgneo_def psdk buildno_header)
