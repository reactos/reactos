
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbddv ${CMAKE_CURRENT_SOURCE_DIR}/kbddv.spec ${CMAKE_CURRENT_BINARY_DIR}/kbddv.def)

add_library(kbddv SHARED kbddv.c kbddv.rc)

set_entrypoint(kbddv 0)

target_link_libraries(kbddv ${CMAKE_CURRENT_BINARY_DIR}/kbddv.def)

add_dependencies(kbddv kbddv_def psdk buildno_header)
