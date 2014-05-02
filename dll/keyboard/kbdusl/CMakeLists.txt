
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusl.dll kbdusl.spec)

add_library(kbdusl SHARED kbdusl.c kbdusl.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdusl.def)

set_module_type(kbdusl kbdlayout)

add_dependencies(kbdusl psdk)

add_cd_file(TARGET kbdusl DESTINATION reactos/system32 FOR all)
