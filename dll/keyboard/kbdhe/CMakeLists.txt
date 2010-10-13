
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdhe ${CMAKE_CURRENT_SOURCE_DIR}/kbdhe.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdhe.def)

add_library(kbdhe SHARED kbdhe.c kbdhe.rc)

set_entrypoint(kbdhe 0)

target_link_libraries(kbdhe ${CMAKE_CURRENT_BINARY_DIR}/kbdhe.def)

add_dependencies(kbdhe kbdhe_def psdk buildno_header)
