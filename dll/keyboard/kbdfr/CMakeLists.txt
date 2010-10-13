
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdfr ${CMAKE_CURRENT_SOURCE_DIR}/kbdfr.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdfr.def)

add_library(kbdfr SHARED kbdfr.c kbdfr.rc)

set_entrypoint(kbdfr 0)

target_link_libraries(kbdfr ${CMAKE_CURRENT_BINARY_DIR}/kbdfr.def)

add_dependencies(kbdfr kbdfr_def psdk buildno_header)
