
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdhu ${CMAKE_CURRENT_SOURCE_DIR}/kbdhu.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdhu.def)

add_library(kbdhu SHARED kbdhu.c kbdhu.rc)

set_entrypoint(kbdhu 0)

target_link_libraries(kbdhu ${CMAKE_CURRENT_BINARY_DIR}/kbdhu.def)

add_dependencies(kbdhu kbdhu_def psdk buildno_header)
