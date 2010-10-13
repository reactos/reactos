
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbda3 ${CMAKE_CURRENT_SOURCE_DIR}/kbda3.spec ${CMAKE_CURRENT_BINARY_DIR}/kbda3.def)

add_library(kbda3 SHARED kbda3.c kbda3.rc)

set_entrypoint(kbda3 0)

target_link_libraries(kbda3 ${CMAKE_CURRENT_BINARY_DIR}/kbda3.def)

add_dependencies(kbda3 kbda3_def psdk buildno_header)
