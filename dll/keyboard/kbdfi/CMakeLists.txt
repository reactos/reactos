
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdfi.dll kbdfi.spec)

add_library(kbdfi SHARED kbdfi.c kbdfi.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdfi.def)

set_entrypoint(kbdfi 0)

add_dependencies(kbdfi psdk)

add_cd_file(TARGET kbdfi DESTINATION reactos/system32 FOR all)
