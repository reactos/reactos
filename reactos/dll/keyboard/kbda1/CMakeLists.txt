
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbda1.dll kbda1.spec)

add_library(kbda1 SHARED kbda1.c kbda1.rc ${CMAKE_CURRENT_BINARY_DIR}/kbda1.def)

set_entrypoint(kbda1 0)

add_dependencies(kbda1 psdk)

add_cd_file(TARGET kbda1 DESTINATION reactos/system32 FOR all)
