
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbur ${CMAKE_CURRENT_SOURCE_DIR}/kbdbur.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdbur.def)

add_library(kbdbur SHARED kbdbur.c kbdbur.rc)

set_entrypoint(kbdbur 0)

target_link_libraries(kbdbur ${CMAKE_CURRENT_BINARY_DIR}/kbdbur.def)

add_dependencies(kbdbur kbdbur_def psdk buildno_header)
