
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdindev.dll kbdindev.spec)

add_library(kbdindev SHARED kbdindev.c kbdindev.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdindev.def)

set_entrypoint(kbdindev 0)

add_dependencies(kbdindev psdk)

add_cd_file(TARGET kbdindev DESTINATION reactos/system32 FOR all)
