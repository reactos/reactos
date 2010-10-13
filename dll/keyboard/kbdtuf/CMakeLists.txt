
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdtuf ${CMAKE_CURRENT_SOURCE_DIR}/kbdtuf.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdtuf.def)

add_library(kbdtuf SHARED kbdtuf.c kbdtuf.rc)

set_entrypoint(kbdtuf 0)

target_link_libraries(kbdtuf ${CMAKE_CURRENT_BINARY_DIR}/kbdtuf.def)

add_dependencies(kbdtuf kbdtuf_def psdk buildno_header)
