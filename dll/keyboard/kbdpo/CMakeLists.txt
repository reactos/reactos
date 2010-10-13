
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdpo ${CMAKE_CURRENT_SOURCE_DIR}/kbdpo.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdpo.def)

add_library(kbdpo SHARED kbdpo.c kbdpo.rc)

set_entrypoint(kbdpo 0)

target_link_libraries(kbdpo ${CMAKE_CURRENT_BINARY_DIR}/kbdpo.def)

add_dependencies(kbdpo kbdpo_def psdk buildno_header)
