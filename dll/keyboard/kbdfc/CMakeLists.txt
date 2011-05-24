
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdfc.dll kbdfc.spec)

add_library(kbdfc SHARED kbdfc.c kbdfc.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdfc.def)

set_entrypoint(kbdfc 0)

add_dependencies(kbdfc psdk)

add_cd_file(TARGET kbdfc DESTINATION reactos/system32 FOR all)
