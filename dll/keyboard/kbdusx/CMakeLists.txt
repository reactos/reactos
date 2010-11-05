
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusx.dll kbdusx.spec)

add_library(kbdusx SHARED kbdusx.c kbdusx.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdusx.def)

set_entrypoint(kbdusx 0)

add_dependencies(kbdusx psdk buildno_header)

add_cab_target(kbdusx 1)