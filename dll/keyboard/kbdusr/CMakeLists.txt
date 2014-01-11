
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusr.dll kbdusr.spec)

add_library(kbdusr SHARED kbdusr.c kbdusr.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdusr.def)

set_module_type(kbdusr kbdlayout)

add_dependencies(kbdusr psdk)

add_cd_file(TARGET kbdusr DESTINATION reactos/system32 FOR all)
