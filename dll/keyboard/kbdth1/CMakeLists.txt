
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdth1.dll kbdth1.spec)

add_library(kbdth1 SHARED kbdth1.c kbdth1.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdth1.def)

set_entrypoint(kbdth1 0)

add_dependencies(kbdth1 psdk)

add_cd_file(TARGET kbdth1 DESTINATION reactos/system32 FOR all)
