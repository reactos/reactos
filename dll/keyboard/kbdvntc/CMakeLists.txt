
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdvntc ${CMAKE_CURRENT_SOURCE_DIR}/kbdvntc.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdvntc.def)

add_library(kbdvntc SHARED kbdvntc.c kbdvntc.rc)

set_entrypoint(kbdvntc 0)

target_link_libraries(kbdvntc ${CMAKE_CURRENT_BINARY_DIR}/kbdvntc.def)

add_dependencies(kbdvntc kbdvntc_def psdk buildno_header)
