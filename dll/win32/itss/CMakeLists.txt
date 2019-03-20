
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(itss.dll itss.spec)

list(APPEND SOURCE
    chm_lib.c
    lzx.c
    itss.c
    moniker.c
    protocol.c
    storage.c
    precomp.h)

add_library(itss MODULE
    ${SOURCE}
    guid.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/itss_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/itss.def)

set_module_type(itss win32dll)
target_link_libraries(itss uuid wine)
add_importlibs(itss urlmon shlwapi ole32 msvcrt kernel32 ntdll)
add_pch(itss precomp.h SOURCE)
add_dependencies(itss wineheaders)
add_cd_file(TARGET itss DESTINATION reactos/system32 FOR all)
