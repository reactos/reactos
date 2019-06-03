
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    browse_ctx.c
    extserv.c
    hlink_main.c
    link.c
    precomp.h)

add_library(hlink MODULE
    ${SOURCE}
    hlink.rc)
spec2def(hlink hlink.dll hlink.spec ADD_IMPORTLIB)

set_module_type(hlink win32dll)
target_link_libraries(hlink uuid wine)
add_delay_importlibs(hlink urlmon)
add_importlibs(hlink shell32 ole32 advapi32 msvcrt kernel32 ntdll)
add_pch(hlink precomp.h SOURCE)
add_cd_file(TARGET hlink DESTINATION reactos/system32 FOR all)
