
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdtuq ${CMAKE_CURRENT_SOURCE_DIR}/kbdtuq.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdtuq.def)

add_library(kbdtuq SHARED kbdtuq.c kbdtuq.rc)

set_entrypoint(kbdtuq 0)

target_link_libraries(kbdtuq ${CMAKE_CURRENT_BINARY_DIR}/kbdtuq.def)

add_dependencies(kbdtuq kbdtuq_def psdk buildno_header)
