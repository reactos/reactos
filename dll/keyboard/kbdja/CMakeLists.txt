
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdja.dll kbdja.spec)

add_library(kbdja SHARED kbdja.c kbdja.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdja.def)

set_entrypoint(kbdja 0)

add_dependencies(kbdja psdk)

add_cd_file(TARGET kbdja DESTINATION reactos/system32 FOR all)
