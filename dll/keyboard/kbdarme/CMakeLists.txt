
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdarme.dll kbdarme.spec)

add_library(kbdarme SHARED kbdarme.c kbdarme.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdarme.def)

set_entrypoint(kbdarme 0)

add_dependencies(kbdarme psdk)

add_cd_file(TARGET kbdarme DESTINATION reactos/system32 FOR all)
