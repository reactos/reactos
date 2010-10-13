
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusr ${CMAKE_CURRENT_SOURCE_DIR}/kbdusr.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdusr.def)

add_library(kbdusr SHARED kbdusr.c kbdusr.rc)

set_entrypoint(kbdusr 0)

target_link_libraries(kbdusr ${CMAKE_CURRENT_BINARY_DIR}/kbdusr.def)

add_dependencies(kbdusr kbdusr_def psdk buildno_header)
