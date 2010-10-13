
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdth0 ${CMAKE_CURRENT_SOURCE_DIR}/kbdth0.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdth0.def)

add_library(kbdth0 SHARED kbdth0.c kbdth0.rc)

set_entrypoint(kbdth0 0)

target_link_libraries(kbdth0 ${CMAKE_CURRENT_BINARY_DIR}/kbdth0.def)

add_dependencies(kbdth0 kbdth0_def psdk buildno_header)
