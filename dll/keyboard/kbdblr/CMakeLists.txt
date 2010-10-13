
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdblr ${CMAKE_CURRENT_SOURCE_DIR}/kbdblr.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdblr.def)

add_library(kbdblr SHARED kbdblr.c kbdblr.rc)

set_entrypoint(kbdblr 0)

target_link_libraries(kbdblr ${CMAKE_CURRENT_BINARY_DIR}/kbdblr.def)

add_dependencies(kbdblr kbdblr_def psdk buildno_header)
