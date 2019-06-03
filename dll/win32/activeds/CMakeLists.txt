
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    activeds_main.c
    stubs.c
    precomp.h)

add_library(activeds MODULE ${SOURCE})
spec2def(activeds activeds.dll activeds.spec)

set_module_type(activeds win32dll)
target_link_libraries(activeds wine)
add_importlibs(activeds msvcrt kernel32 ntdll)
add_pch(activeds precomp.h SOURCE)
add_cd_file(TARGET activeds DESTINATION reactos/system32 FOR all)
