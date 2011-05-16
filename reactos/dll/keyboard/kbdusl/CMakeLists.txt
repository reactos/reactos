
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdusl.dll kbdusl.spec)

add_library(kbdusl SHARED kbdusl.c kbdusl.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdusl.def)

set_entrypoint(kbdusl 0)

add_dependencies(kbdusl psdk)

add_cd_file(TARGET kbdusl DESTINATION reactos/system32 FOR all)
