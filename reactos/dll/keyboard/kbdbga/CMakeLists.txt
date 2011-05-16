
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbga.dll kbdbga.spec)

add_library(kbdbga SHARED kbdbga.c kbdbga.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdbga.def)

set_entrypoint(kbdbga 0)

add_dependencies(kbdbga psdk)

add_cd_file(TARGET kbdbga DESTINATION reactos/system32 FOR all)
