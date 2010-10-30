
include_directories(
    include
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/adns/src
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/adns/adns_win32)

add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-DADNS_JGAA_WIN32)

spec2def(dnsapi.dll dnsapi.spec)

list(APPEND SOURCE 
    dnsapi/adns.c
    dnsapi/context.c
    dnsapi/memory.c
    dnsapi/names.c
    dnsapi/query.c
    dnsapi/record.c
    dnsapi/stubs.c
    dnsapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dnsapi.def)

add_library(dnsapi SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/dnsapi_precomp.h.gch
    ${SOURCE})

set_entrypoint(dnsapi 0)

target_link_libraries(dnsapi adns)

add_importlibs(dnsapi user32 ws2_32 iphlpapi msvcrt kernel32 ntdll)
add_pch(dnsapi ${CMAKE_CURRENT_SOURCE_DIR}/dnsapi/precomp.h ${SOURCE})
add_dependencies(dnsapi psdk buildno_header)
add_cab_target(dnsapi 1)