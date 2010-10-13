
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdus ${CMAKE_CURRENT_SOURCE_DIR}/kbdus.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdus.def)

add_library(kbdus SHARED kbdus.c kbdus.rc)

set_entrypoint(kbdus 0)

target_link_libraries(kbdus ${CMAKE_CURRENT_BINARY_DIR}/kbdus.def)

add_dependencies(kbdus kbdus_def psdk buildno_header)
