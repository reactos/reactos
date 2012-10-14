
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbda3.dll kbda3.spec)

add_library(kbda3 SHARED kbda3.c kbda3.rc ${CMAKE_CURRENT_BINARY_DIR}/kbda3.def)

set_entrypoint(kbda3 0)

add_dependencies(kbda3 psdk)

add_cd_file(TARGET kbda3 DESTINATION reactos/system32 FOR all)
