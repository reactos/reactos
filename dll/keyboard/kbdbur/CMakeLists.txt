
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdbur.dll kbdbur.spec)

add_library(kbdbur SHARED kbdbur.c kbdbur.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdbur.def)

set_entrypoint(kbdbur 0)

add_dependencies(kbdbur psdk)

add_cd_file(TARGET kbdbur DESTINATION reactos/system32 FOR all)
