
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdth0.dll kbdth0.spec)

add_library(kbdth0 SHARED kbdth0.c kbdth0.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdth0.def)

set_entrypoint(kbdth0 0)



add_dependencies(kbdth0 psdk buildno_header)
