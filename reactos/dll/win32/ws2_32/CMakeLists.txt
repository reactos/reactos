
add_definitions(-DLE)
add_definitions(-DWINSOCK_API_LINKAGE=DECLSPEC_EXPORT)
include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(ws2_32.dll ws2_32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    misc/bsd.c
    misc/catalog.c
    misc/dllmain.c
    misc/event.c
    misc/handle.c
    misc/ns.c
    misc/sndrcv.c
    misc/stubs.c
    misc/upcall.c
    include/ws2_32.h)

add_library(ws2_32 SHARED
    ${SOURCE}
    wine/async.c
    wine/socket.c
    ws2_32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ws2_32.def)

set_module_type(ws2_32 win32dll UNICODE)
target_link_libraries(ws2_32 wine)
add_delay_importlibs(ws2_32 user32)
add_importlibs(ws2_32 advapi32 dnsapi ws2help msvcrt kernel32 ntdll)
add_pch(ws2_32 include/ws2_32.h SOURCE)
add_cd_file(TARGET ws2_32 DESTINATION reactos/system32 FOR all)
