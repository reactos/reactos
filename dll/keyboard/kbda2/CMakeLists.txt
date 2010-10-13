
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbda2 ${CMAKE_CURRENT_SOURCE_DIR}/kbda2.spec ${CMAKE_CURRENT_BINARY_DIR}/kbda2.def)

add_library(kbda2 SHARED kbda2.c kbda2.rc)

set_entrypoint(kbda2 0)

target_link_libraries(kbda2 ${CMAKE_CURRENT_BINARY_DIR}/kbda2.def)

add_dependencies(kbda2 kbda2_def psdk buildno_header)
