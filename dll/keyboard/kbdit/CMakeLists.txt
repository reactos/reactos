
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdit ${CMAKE_CURRENT_SOURCE_DIR}/kbdit.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdit.def)

add_library(kbdit SHARED kbdit.c kbdit.rc)

set_entrypoint(kbdit 0)

target_link_libraries(kbdit ${CMAKE_CURRENT_BINARY_DIR}/kbdit.def)

add_dependencies(kbdit kbdit_def psdk buildno_header)
