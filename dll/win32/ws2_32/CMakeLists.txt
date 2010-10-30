
set_unicode()

add_definitions(-DLE)
add_definitions(-DWINSOCK_API_LINKAGE=DECLSPEC_EXPORT)

include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(ws2_32 ${CMAKE_CURRENT_SOURCE_DIR}/ws2_32.spec ${CMAKE_CURRENT_BINARY_DIR}/ws2_32.def)

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
    misc/async.c
    ws2_32.rc)

add_library(ws2_32 SHARED ${CMAKE_CURRENT_BINARY_DIR}/ws2_32_ws2_32.h.gch ${SOURCE})

set_module_type(ws2_32 win32dll)

target_link_libraries(ws2_32
     ${CMAKE_CURRENT_BINARY_DIR}/ws2_32.def
     wine)

add_importlibs(ws2_32 user32 advapi32 dnsapi ws2help kernel32 ntdll)
add_pch(ws2_32 ${CMAKE_CURRENT_SOURCE_DIR}/include/ws2_32.h ${SOURCE})
add_dependencies(ws2_32 ws2_32_def)
add_cab_target(ws2_32 1)
