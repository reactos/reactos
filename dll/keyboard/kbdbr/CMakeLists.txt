
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbr ${CMAKE_CURRENT_SOURCE_DIR}/kbdbr.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdbr.def)

add_library(kbdbr SHARED kbdbr.c kbdbr.rc)

set_entrypoint(kbdbr 0)

target_link_libraries(kbdbr ${CMAKE_CURRENT_BINARY_DIR}/kbdbr.def)

add_dependencies(kbdbr kbdbr_def psdk buildno_header)
