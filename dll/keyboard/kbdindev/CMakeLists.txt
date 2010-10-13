
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdindev ${CMAKE_CURRENT_SOURCE_DIR}/kbdindev.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdindev.def)

add_library(kbdindev SHARED kbdindev.c kbdindev.rc)

set_entrypoint(kbdindev 0)

target_link_libraries(kbdindev ${CMAKE_CURRENT_BINARY_DIR}/kbdindev.def)

add_dependencies(kbdindev kbdindev_def psdk buildno_header)
