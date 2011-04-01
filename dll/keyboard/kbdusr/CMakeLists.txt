
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusr.dll kbdusr.spec)

add_library(kbdusr SHARED kbdusr.c kbdusr.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdusr.def)

set_entrypoint(kbdusr 0)

add_dependencies(kbdusr psdk)

add_cab_target(kbdusr 1)
