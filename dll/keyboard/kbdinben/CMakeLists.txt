
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdinben ${CMAKE_CURRENT_SOURCE_DIR}/kbdinben.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdinben.def)

add_library(kbdinben SHARED kbdinben.c kbdinben.rc)

set_entrypoint(kbdinben 0)

target_link_libraries(kbdinben ${CMAKE_CURRENT_BINARY_DIR}/kbdinben.def)

add_dependencies(kbdinben kbdinben_def psdk buildno_header)
