
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdinmal ${CMAKE_CURRENT_SOURCE_DIR}/kbdinmal.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdinmal.def)

add_library(kbdinmal SHARED kbdinmal.c kbdinmal.rc)

set_entrypoint(kbdinmal 0)

target_link_libraries(kbdinmal ${CMAKE_CURRENT_BINARY_DIR}/kbdinmal.def)

add_dependencies(kbdinmal kbdinmal_def psdk buildno_header)
