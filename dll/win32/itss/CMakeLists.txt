
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

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
    rsrc.rc)
spec2def(itss itss.dll itss.spec)

set_module_type(itss win32dll)
target_link_libraries(itss uuid wine)
add_importlibs(itss urlmon shlwapi ole32 msvcrt kernel32 ntdll)
add_pch(itss precomp.h SOURCE)
add_dependencies(itss wineheaders)
add_cd_file(TARGET itss DESTINATION reactos/system32 FOR all)
