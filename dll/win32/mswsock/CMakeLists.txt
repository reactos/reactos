
add_definitions(-DLE)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mswsock.dll mswsock.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    extensions.c
    mswhelper.c
    nsplookup.c
    stubs.c
    precomp.h)

add_library(mswsock SHARED
    ${SOURCE}
    mswsock.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mswsock.def)

set_module_type(mswsock win32dll UNICODE)
add_importlibs(mswsock ws2_32 advapi32 dnsapi msvcrt kernel32 ntdll)
add_pch(mswsock precomp.h SOURCE)
add_cd_file(TARGET mswsock DESTINATION reactos/system32 FOR all)
