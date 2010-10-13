
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdur ${CMAKE_CURRENT_SOURCE_DIR}/kbdur.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdur.def)

add_library(kbdur SHARED kbdur.c kbdur.rc)

set_entrypoint(kbdur 0)

target_link_libraries(kbdur ${CMAKE_CURRENT_BINARY_DIR}/kbdur.def)

add_dependencies(kbdur kbdur_def psdk buildno_header)
