
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdkaz ${CMAKE_CURRENT_SOURCE_DIR}/kbdkaz.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdkaz.def)

add_library(kbdkaz SHARED kbdkaz.c kbdkaz.rc)

set_entrypoint(kbdkaz 0)

target_link_libraries(kbdkaz ${CMAKE_CURRENT_BINARY_DIR}/kbdkaz.def)

add_dependencies(kbdkaz kbdkaz_def psdk buildno_header)
