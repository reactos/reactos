
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbda1 ${CMAKE_CURRENT_SOURCE_DIR}/kbda1.spec ${CMAKE_CURRENT_BINARY_DIR}/kbda1.def)

add_library(kbda1 SHARED kbda1.c kbda1.rc)

set_entrypoint(kbda1 0)

target_link_libraries(kbda1 ${CMAKE_CURRENT_BINARY_DIR}/kbda1.def)

add_dependencies(kbda1 kbda1_def psdk buildno_header)
