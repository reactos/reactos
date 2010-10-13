
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusx ${CMAKE_CURRENT_SOURCE_DIR}/kbdusx.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdusx.def)

add_library(kbdusx SHARED kbdusx.c kbdusx.rc)

set_entrypoint(kbdusx 0)

target_link_libraries(kbdusx ${CMAKE_CURRENT_BINARY_DIR}/kbdusx.def)

add_dependencies(kbdusx kbdusx_def psdk buildno_header)
