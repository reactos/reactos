
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbgt ${CMAKE_CURRENT_SOURCE_DIR}/kbdbgt.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdbgt.def)

add_library(kbdbgt SHARED kbdbgt.c kbdbgt.rc)

set_entrypoint(kbdbgt 0)

target_link_libraries(kbdbgt ${CMAKE_CURRENT_BINARY_DIR}/kbdbgt.def)

add_dependencies(kbdbgt kbdbgt_def psdk buildno_header)
