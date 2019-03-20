
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(tapi32.dll tapi32.spec)

list(APPEND SOURCE
    assisted.c
    internal.c
    line.c
    phone.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/tapi32_stubs.c)

add_library(tapi32 MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/tapi32.def)

set_module_type(tapi32 win32dll)
target_link_libraries(tapi32 wine)
add_importlibs(tapi32 advapi32 msvcrt kernel32 ntdll)
add_pch(tapi32 precomp.h SOURCE)
add_cd_file(TARGET tapi32 DESTINATION reactos/system32 FOR all)
