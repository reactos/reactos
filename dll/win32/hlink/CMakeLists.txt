
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(hlink.dll hlink.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    browse_ctx.c
    extserv.c
    hlink_main.c
    link.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/hlink_stubs.c)

add_library(hlink MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    hlink.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hlink.def)

set_module_type(hlink win32dll)
target_link_libraries(hlink uuid wine oldnames)
add_delay_importlibs(hlink urlmon)
add_importlibs(hlink shell32 ole32 advapi32 msvcrt kernel32 ntdll)
add_pch(hlink precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET hlink DESTINATION reactos/system32 FOR all)
