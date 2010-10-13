
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdfc ${CMAKE_CURRENT_SOURCE_DIR}/kbdfc.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdfc.def)

add_library(kbdfc SHARED kbdfc.c kbdfc.rc)

set_entrypoint(kbdfc 0)

target_link_libraries(kbdfc ${CMAKE_CURRENT_BINARY_DIR}/kbdfc.def)

add_dependencies(kbdfc kbdfc_def psdk buildno_header)
