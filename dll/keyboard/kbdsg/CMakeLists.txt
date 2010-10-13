
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdsg ${CMAKE_CURRENT_SOURCE_DIR}/kbdsg.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdsg.def)

add_library(kbdsg SHARED kbdsg.c kbdsg.rc)

set_entrypoint(kbdsg 0)

target_link_libraries(kbdsg ${CMAKE_CURRENT_BINARY_DIR}/kbdsg.def)

add_dependencies(kbdsg kbdsg_def psdk buildno_header)
