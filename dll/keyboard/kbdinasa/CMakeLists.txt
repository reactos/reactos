
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdinasa ${CMAKE_CURRENT_SOURCE_DIR}/kbdinasa.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdinasa.def)

add_library(kbdinasa SHARED kbdinasa.c kbdinasa.rc)

set_entrypoint(kbdinasa 0)

target_link_libraries(kbdinasa ${CMAKE_CURRENT_BINARY_DIR}/kbdinasa.def)

add_dependencies(kbdinasa kbdinasa_def psdk buildno_header)
