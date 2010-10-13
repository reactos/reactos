
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusl ${CMAKE_CURRENT_SOURCE_DIR}/kbdusl.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdusl.def)

add_library(kbdusl SHARED kbdusl.c kbdusl.rc)

set_entrypoint(kbdusl 0)

target_link_libraries(kbdusl ${CMAKE_CURRENT_BINARY_DIR}/kbdusl.def)

add_dependencies(kbdusl kbdusl_def psdk buildno_header)
