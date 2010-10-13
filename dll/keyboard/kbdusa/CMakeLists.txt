
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusa ${CMAKE_CURRENT_SOURCE_DIR}/kbdusa.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdusa.def)

add_library(kbdusa SHARED kbdusa.c kbdusa.rc)

set_entrypoint(kbdusa 0)

target_link_libraries(kbdusa ${CMAKE_CURRENT_BINARY_DIR}/kbdusa.def)

add_dependencies(kbdusa kbdusa_def psdk buildno_header)
