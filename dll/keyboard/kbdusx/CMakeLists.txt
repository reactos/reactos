
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusx.dll kbdusx.spec)

add_library(kbdusx SHARED kbdusx.c kbdusx.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdusx.def)

set_module_type(kbdusx kbdlayout)

add_dependencies(kbdusx psdk)

add_cd_file(TARGET kbdusx DESTINATION reactos/system32 FOR all)
