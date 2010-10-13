
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdir ${CMAKE_CURRENT_SOURCE_DIR}/kbdir.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdir.def)

add_library(kbdir SHARED kbdir.c kbdir.rc)

set_entrypoint(kbdir 0)

target_link_libraries(kbdir ${CMAKE_CURRENT_BINARY_DIR}/kbdir.def)

add_dependencies(kbdir kbdir_def psdk buildno_header)
