
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdsk ${CMAKE_CURRENT_SOURCE_DIR}/kbdsk.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdsk.def)

add_library(kbdsk SHARED kbdsk.c kbdsk.rc)

set_entrypoint(kbdsk 0)

target_link_libraries(kbdsk ${CMAKE_CURRENT_BINARY_DIR}/kbdsk.def)

add_dependencies(kbdsk kbdsk_def psdk buildno_header)
