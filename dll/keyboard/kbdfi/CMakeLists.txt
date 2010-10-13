
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdfi ${CMAKE_CURRENT_SOURCE_DIR}/kbdfi.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdfi.def)

add_library(kbdfi SHARED kbdfi.c kbdfi.rc)

set_entrypoint(kbdfi 0)

target_link_libraries(kbdfi ${CMAKE_CURRENT_BINARY_DIR}/kbdfi.def)

add_dependencies(kbdfi kbdfi_def psdk buildno_header)
