
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(mapi32.dll mapi32.spec)

list(APPEND SOURCE
    imalloc.c
    mapi32_main.c
    prop.c
    sendmail.c
    stubs.c
    util.c
    precomp.h)

add_library(mapi32 SHARED
    ${SOURCE}
    guid.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mapi32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mapi32.def)

set_module_type(mapi32 win32dll)
target_link_libraries(mapi32 uuid wine)
add_importlibs(mapi32 shlwapi user32 advapi32 msvcrt kernel32 ntdll)
add_pch(mapi32 precomp.h SOURCE)
add_cd_file(TARGET mapi32 DESTINATION reactos/system32 FOR all)
