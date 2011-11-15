
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdpo.dll kbdpo.spec)

add_library(kbdpo SHARED kbdpo.c kbdpo.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdpo.def)

set_entrypoint(kbdpo 0)

add_dependencies(kbdpo psdk)

add_cd_file(TARGET kbdpo DESTINATION reactos/system32 FOR all)
