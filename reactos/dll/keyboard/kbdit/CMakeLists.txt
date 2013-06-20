
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdit.dll kbdit.spec)

add_library(kbdit SHARED kbdit.c kbdit.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdit.def)

set_entrypoint(kbdit 0)

add_dependencies(kbdit psdk)

add_cd_file(TARGET kbdit DESTINATION reactos/system32 FOR all)
