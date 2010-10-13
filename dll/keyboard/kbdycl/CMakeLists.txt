
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdycl ${CMAKE_CURRENT_SOURCE_DIR}/kbdycl.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdycl.def)

add_library(kbdycl SHARED kbdycl.c kbdycl.rc)

set_entrypoint(kbdycl 0)

target_link_libraries(kbdycl ${CMAKE_CURRENT_BINARY_DIR}/kbdycl.def)

add_dependencies(kbdycl kbdycl_def psdk buildno_header)
