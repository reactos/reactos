
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdarme ${CMAKE_CURRENT_SOURCE_DIR}/kbdarme.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdarme.def)

add_library(kbdarme SHARED kbdarme.c kbdarme.rc)

set_entrypoint(kbdarme 0)

target_link_libraries(kbdarme ${CMAKE_CURRENT_BINARY_DIR}/kbdarme.def)

add_dependencies(kbdarme kbdarme_def psdk buildno_header)
