
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbgm ${CMAKE_CURRENT_SOURCE_DIR}/kbdbgm.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdbgm.def)

add_library(kbdbgm SHARED kbdbgm.c kbdbgm.rc)

set_entrypoint(kbdbgm 0)

target_link_libraries(kbdbgm ${CMAKE_CURRENT_BINARY_DIR}/kbdbgm.def)

add_dependencies(kbdbgm kbdbgm_def psdk buildno_header)
