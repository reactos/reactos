
add_definitions(-D__WINESRC__ -D__ROS_LONG64__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(itss.dll itss.spec)

list(APPEND SOURCE
    chm_lib.c
    lzx.c
    itss.c
    moniker.c
    protocol.c
    storage.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/itss_stubs.c)

add_library(itss MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/itss.def)

set_module_type(itss win32dll)
target_link_libraries(itss uuid wine oldnames)
add_importlibs(itss urlmon shlwapi ole32 msvcrt kernel32 ntdll)
add_pch(itss precomp.h "${PCH_SKIP_SOURCE}")
add_dependencies(itss wineheaders)
add_cd_file(TARGET itss DESTINATION reactos/system32 FOR all)
