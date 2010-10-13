
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdlt1 ${CMAKE_CURRENT_SOURCE_DIR}/kbdlt1.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdlt1.def)

add_library(kbdlt1 SHARED kbdlt1.c kbdlt1.rc)

set_entrypoint(kbdlt1 0)

target_link_libraries(kbdlt1 ${CMAKE_CURRENT_BINARY_DIR}/kbdlt1.def)

add_dependencies(kbdlt1 kbdlt1_def psdk buildno_header)
