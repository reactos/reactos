
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdazel ${CMAKE_CURRENT_SOURCE_DIR}/kbdazel.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdazel.def)

add_library(kbdazel SHARED kbdazel.c kbdazel.rc)

set_entrypoint(kbdazel 0)

target_link_libraries(kbdazel ${CMAKE_CURRENT_BINARY_DIR}/kbdazel.def)

add_dependencies(kbdazel kbdazel_def psdk buildno_header)
